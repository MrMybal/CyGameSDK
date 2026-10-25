"""Interface declarations, nominal signatures, native dispatch and official Core path."""
import copy,json,os,shutil,subprocess,sys,tempfile,unittest
from pathlib import Path
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import tool,test_integration as integration,test_authoring as authoring
from cytools_core import CyToolError

def port(type='float',value=0):return {'type':type,'default':value}
def contract(mode='pure',inputs=None,outputs=None):return {'methods':{'Read':{'mode':mode,'inputs':inputs if inputs is not None else {'amount':port(value=7)},'outputs':outputs if outputs is not None else {'result':port()}}},'metadata':{'keep':23}}
class Interfaces(unittest.TestCase):
 @classmethod
 def setUpClass(cls):integration.EditorIntegration.setUpClass.__func__(cls)
 tearDownClass=classmethod(integration.EditorIntegration.tearDownClass.__func__)
 call=integration.EditorIntegration.call;graph=authoring.NativeAuthoring.graph;wire=authoring.NativeAuthoring.wire;unlink=authoring.NativeAuthoring.unlink;function=authoring.NativeAuthoring.function
 def get(self,g,fn=''):return self.call('graph_get',{'graphId':g,**({'functionName':fn} if fn else {})})['graph']
 def node(self,g,type,**values):
  p={'graphId':g,'type':type,'position':[100,100]};p.update({k:values.pop(k) for k in ('interfaceRef','methodRef','classRef','functionRef','functionName','typeRef','fieldRef') if k in values});n=self.call('graph_node_add',p)['node']['id']
  for k,v in values.items():self.call('graph_node_value',{'graphId':g,'nodeId':n,'key':k,'value':v,**({'functionName':p['functionName']} if 'functionName' in p else {})})
  return n
 def define(self,g,name='Readable',definition=None):return self.call('graph_interface_define',{'graphId':g,'name':name,'definition':definition or contract()})
 def cls(self,g,name,methods=None,interfaces=None,parent='',fields=None):
  p={'graphId':g,'name':name,'parent':parent,'fields':fields or {},'methods':methods or {}}
  if interfaces is not None:p['interfaces']=interfaces
  return self.call('graph_class_define',p)
 def method(self,g,name,add=0,mode='pure'):
  self.function(g,name,{'self':('object',''),'amount':('float',99)},{'result':('float',0)},mode=mode)
  plus=self.node(g,'math.add',b=add,functionName=name);self.wire(g,'arguments','amount',plus,'a',name);self.wire(g,plus,'value','return','result',name)
  if mode=='execution':self.wire(g,'arguments','then','return','exec',name)
 def output(self,g,n,pin='result'):
  text=self.node(g,'convert.float_string');self.wire(g,n,pin,text,'value');self.wire(g,text,'value','print','text')
 def flow(self,g,cls='Base',execution=False):
  obj=self.node(g,'object.new',classRef=cls);call=self.node(g,'interface.call',interfaceRef='Readable',methodRef='Read');self.wire(g,obj,'object',call,'self');self.output(g,call);self.unlink(g,'start');self.wire(g,'begin','then',obj,'exec');self.wire(g,obj,'then',call if execution else 'print','exec')
  if execution:self.wire(g,call,'then','print','exec')
  return obj,call
 def test_01_pure_dispatch_across_unrelated_classes_inheritance_and_contract_defaults(self):
  g=self.graph('InterfacesPure');self.define(g);self.method(g,'BaseRead',10);self.method(g,'OtherRead',100);self.cls(g,'Base',{'Read':'BaseRead'},['Readable']);self.cls(g,'Derived',{'Read':'OtherRead'},parent='Base');self.cls(g,'Unrelated',{'Read':'OtherRead'},['Readable']);obj,call=self.flow(g,cls='Derived')
  self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['107'])
  for name,expected in [('Base','17'),('Unrelated','107')]:
   self.call('graph_node_value',{'graphId':g,'nodeId':obj,'key':'class','value':name});self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],[expected])
  self.assertEqual(self.call('graph_get',{'graphId':g})['nodeDefinitions'][call]['inputs'],{'self':'object','amount':'float'})
 def test_02_execution_dispatch_step_into_and_actual_method_stack(self):
  g=self.graph('InterfacesExec');self.define(g,definition=contract('execution'));self.method(g,'ExecuteRead',20,'execution');self.cls(g,'Base',{'Read':'ExecuteRead'},['Readable']);self.flow(g,execution=True)
  self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['27']);self.call('graph_debug_start',{'graphId':g})
  entered=False
  for _ in range(8):
   state=self.call('graph_debug_step_into',{'graphId':g});entered|=state.get('activeFunction')=='ExecuteRead'
   if entered:break
  self.assertTrue(entered,state);state=self.call('graph_debug_continue',{'graphId':g});self.assertEqual(state['messages'],['27']);self.assertEqual(state['state'],'Completed')
 def test_03_supports_invalid_receiver_missing_implementation_and_destroyed_object(self):
  g=self.graph('InterfacesReceiver');self.define(g);self.method(g,'ReadImpl');self.cls(g,'Base',{'Read':'ReadImpl'},['Readable']);self.cls(g,'Plain');obj,call=self.flow(g)
  self.call('graph_node_value',{'graphId':g,'nodeId':obj,'key':'class','value':'Plain'})
  with self.assertRaises(CyToolError):self.call('graph_run',{'graphId':g})
  self.function(g,'Check',{'peer':('object','')},{'ok':('bool',True)})
  check=self.node(g,'interface.supports',interfaceRef='Readable',functionName='Check');self.wire(g,'arguments','peer',check,'self','Check');self.wire(g,check,'value','return','ok','Check')
  self.assertFalse(self.call('graph_function_evaluate',{'graphId':g,'name':'Check','arguments':{'peer':'object:unknown'}})['values']['ok'])
  self.call('graph_node_value',{'graphId':g,'nodeId':obj,'key':'class','value':'Base'});destroy=self.node(g,'object.destroy');self.wire(g,obj,'object',destroy,'object')
  link=next(l for l in self.get(g)['links'] if l['fromNode']==obj and l['fromPin']=='then');self.unlink(g,link['id']);self.wire(g,obj,'then',destroy,'exec');self.wire(g,destroy,'then','print','exec')
  with self.assertRaises(CyToolError):self.call('graph_run',{'graphId':g})
 def test_04_signature_validation_dependency_removal_and_atomic_undo(self):
  g=self.graph('InterfacesAtomic');self.define(g);self.method(g,'Impl');self.cls(g,'Base',{'Read':'Impl'},['Readable']);before=self.get(g);rev=self.call('editor_status')['revision'];history=self.call('edit_history')
  for operation,p in [('graph_interface_remove',{'name':'Readable'}),('graph_interface_define',{'name':'Readable','definition':contract('execution')}),('graph_class_define',{'name':'Bad','fields':{},'interfaces':['Readable']}),('graph_class_define',{'name':'Bad','fields':{},'interfaces':['Missing']}),('graph_class_define',{'name':'Bad','fields':{},'methods':{'Read':'Impl'},'interfaces':['Readable','Readable']})]:
   with self.assertRaises(CyToolError):self.call(operation,{'graphId':g,**p})
  self.assertEqual(self.get(g),before);self.assertEqual(self.call('editor_status')['revision'],rev);self.assertEqual(self.call('edit_history'),history)
  self.define(g,'Unused');self.call('edit_undo');self.assertNotIn('Unused',self.get(g)['interfaces']);self.call('edit_redo');self.assertIn('Unused',self.get(g)['interfaces']);self.call('graph_interface_remove',{'graphId':g,'name':'Unused'})
  # Existing source callers that omit interfaces preserve the established contract.
  self.cls(g,'Base',{'Read':'Impl'});self.assertEqual(self.get(g)['classes']['Base']['interfaces'],['Readable'])
 def test_05_dynamic_recursion_is_rejected_before_commit(self):
  g=self.graph('InterfacesRecursion');self.define(g);self.method(g,'Impl');self.cls(g,'Base',{'Read':'Impl'},['Readable']);before=self.get(g)
  with self.assertRaises(CyToolError):self.node(g,'interface.call',interfaceRef='Readable',methodRef='Read',functionName='Impl')
  self.assertEqual(self.get(g),before)
 def test_06_named_types_nested_function_macro_and_clipboard_into_function(self):
  g=self.graph('InterfacesNamed');self.call('graph_type_define',{'graphId':g,'typeId':'struct.Info','definition':{'kind':'struct','fields':{'hp':port(value=42)}}});value={'hp':42};spec=contract(inputs={'data':port('struct.Info',value)},outputs={'result':port('struct.Info',value)});self.define(g,definition=spec)
  self.function(g,'Impl',{'self':('object',''),'data':('struct.Info',value)},{'result':('struct.Info',value)});self.wire(g,'arguments','data','return','result','Impl');self.cls(g,'Base',{'Read':'Impl'},['Readable'])
  self.function(g,'Inline',{'peer':('object',''),'data':('struct.Info',value)},{'result':('struct.Info',value)},kind='macro');call=self.node(g,'interface.call',interfaceRef='Readable',methodRef='Read',functionName='Inline');self.wire(g,'arguments','peer',call,'self','Inline');self.wire(g,'arguments','data',call,'data','Inline');self.wire(g,call,'result','return','result','Inline')
  self.function(g,'Wrapper',{'peer':('object','')},{'result':('struct.Info',value)});macro=self.node(g,'macro.call',functionRef='Inline',functionName='Wrapper');self.wire(g,'arguments','peer',macro,'peer','Wrapper');self.wire(g,macro,'result','return','result','Wrapper')
  obj=self.node(g,'object.new',classRef='Base');root=self.node(g,'function.call',functionRef='Wrapper');self.wire(g,obj,'object',root,'peer');split=self.node(g,'struct.break',typeRef='struct.Info');self.wire(g,root,'result',split,'value');self.output(g,split,'hp');self.unlink(g,'start');self.wire(g,'begin','then',obj,'exec');self.wire(g,obj,'then','print','exec');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['42'])
  fragment=self.call('graph_fragment_copy',{'graphId':g,'nodeIds':[obj,root]})['fragment'];other=self.graph('InterfacesPaste');self.function(other,'Container',{}, {'result':('float',0)},mode='execution')
  self.call('graph_fragment_paste',{'graphId':other,'functionName':'Container','fragment':fragment});owner=self.get(other);self.assertEqual(owner['interfaces'],self.get(g)['interfaces']);self.assertNotIn('interfaces',owner['functions']['Container']);self.assertNotIn('interfaces',self.get(g)['functions']['Wrapper'])
  # A different interface declaration with the same identity rejects the whole paste.
  conflict=self.graph('InterfacesConflict');self.define(conflict,definition=contract());before=self.get(conflict)
  with self.assertRaises(CyToolError):self.call('graph_fragment_paste',{'graphId':conflict,'fragment':fragment})
  self.assertEqual(self.get(conflict),before)
 def test_07_declaration_schema_limits_material_domain_and_named_dependencies(self):
  g=self.graph('InterfacesInvalid');before=self.get(g)
  for name,definition in [('Bad name',contract()),('Empty',{'methods':{}}),('Body',{'methods':{'Read':{**contract()['methods']['Read'],'nodes':[]}}}),('Self',contract(inputs={'self':port('object','')})),('Mode',contract('unknown')),('NoOutput',contract(outputs={})),('Many',{'methods':{str(i):contract()['methods']['Read'] for i in range(17)}})]:
   with self.assertRaises(CyToolError):self.define(g,name,definition)
  self.assertEqual(self.get(g),before)
  material=self.call('asset_new_graph',{'name':'InterfacesMaterial','domain':'material'})['graphId']
  with self.assertRaises(CyToolError):self.define(material)
  self.call('graph_type_define',{'graphId':g,'typeId':'enum.Team','definition':{'kind':'enum','members':['One'],'default':'One'}});self.define(g,definition=contract(inputs={'team':port('enum.Team','One')}))
  with self.assertRaises(CyToolError):self.call('graph_type_remove',{'graphId':g,'typeId':'enum.Team'})
 def test_08_save_reload_metadata_debug_stale_and_official_core(self):
  g=self.graph('InterfacesPersistence');self.define(g);self.method(g,'Impl');self.cls(g,'Base',{'Read':'Impl'},['Readable']);self.flow(g);self.call('asset_save_graph',{'graphId':g});path=self.get(g)['assetPath'];saved=json.loads((self.workspace/'data/assets'/path).read_text('utf-8'));self.assertEqual(saved['graph']['interfaces']['Readable']['metadata'],{'keep':23});self.call('asset_open',{'path':path});self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['7'])
  self.call('graph_debug_start',{'graphId':g});self.define(g,'Other');self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Stale')
  with tool.create_runtime(self.workspace/'interface-core') as runtime:
   actor=runtime.authenticate(runtime.create_client('Interfaces')['token']);session=runtime.create_session(actor);job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],'graph_interfaces_get',{'graphId':g})['jobId']);self.assertEqual(job['state'],'Completed',job);self.assertIn('Readable',job['outputs']['interfaces']);runtime.finish_task(actor,session['sessionId'])

 def test_09_native_interface_creation_and_signature_gestures_en_fr(self):
  if not self.gui:self.skipTest('Graphical host required')
  for lang in ('en','fr'):
   with tempfile.TemporaryDirectory(prefix='interface-ui-',dir=ROOT/'data/reports') as tmp:
    root=Path(tmp);(root/'app').mkdir();shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copytree(APP/'locales',root/'app/locales');report=root/'report.json'
    process=subprocess.run([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Scripting','--lang',lang,'--test-interfaces',str(report),'--exit-after','40'],timeout=55)
    self.assertEqual(process.returncode,0);self.assertTrue(report.exists());result=json.loads(report.read_text('utf-8'));self.assertTrue(result['passed'],result);self.assertEqual(len(result['checks']),8)
    shutil.copyfile(report,ROOT/'data/reports'/('interfaces-gestures-'+lang+'.json'));self.assertFalse((root/'data/settings/ui-preferences.json').exists());self.assertFalse((root/'data/settings/editor.ini').exists())

 def test_10_latent_interface_mutation_shared_objects_and_cached_return(self):
  g=self.graph('InterfacesLatent');self.define(g,definition=contract('execution'));self.function(g,'WaitRead',{'self':('object',''),'amount':('float',99)},{'result':('float',0)},mode='execution')
  setfield=self.node(g,'object.set.float',field='health',functionName='WaitRead');delay=self.node(g,'flow.delay',seconds=.1,functionName='WaitRead');self.wire(g,'arguments','self',setfield,'object','WaitRead');self.wire(g,'arguments','amount',setfield,'value','WaitRead');self.wire(g,'arguments','then',setfield,'exec','WaitRead');self.wire(g,setfield,'then',delay,'exec','WaitRead');self.wire(g,delay,'then','return','exec','WaitRead');self.wire(g,setfield,'value','return','result','WaitRead');self.cls(g,'Base',{'Read':'WaitRead'},['Readable'],fields={'health':port(value=0)});self.flow(g,execution=True)
  self.call('graph_debug_start',{'graphId':g});state=self.call('graph_debug_continue',{'graphId':g});self.assertEqual(state['state'],'Waiting');self.assertEqual(next(iter(state['objects'].values()))['fields']['health'],7);state=self.call('graph_debug_tick',{'graphId':g,'delta':.2});self.assertEqual(state['state'],'Completed',state);self.assertEqual(state['messages'],['7']);self.assertEqual(self.get(g)['classes']['Base']['fields']['health']['default'],0)
 def test_11_nominal_signatures_multicontract_conflicts_and_function_rename(self):
  g=self.graph('InterfacesContracts');self.define(g);self.method(g,'Impl');self.cls(g,'Base',{'Read':'Impl'},['Readable']);other=contract(inputs={'amount':port('int',7)});self.define(g,'Other',other);before=self.get(g)
  with self.assertRaises(CyToolError):self.cls(g,'Base',{'Read':'Impl'},['Readable','Other'])
  self.assertEqual(self.get(g),before)
  self.flow(g);self.call('graph_function_rename',{'graphId':g,'name':'Impl','newName':'Renamed'});self.assertEqual(self.get(g)['classes']['Base']['methods']['Read'],'Renamed');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['7'])
  before=self.get(g)
  with self.assertRaises(CyToolError):self.call('graph_function_remove',{'graphId':g,'name':'Renamed'})
  self.assertEqual(self.get(g),before)

if __name__=='__main__':unittest.main(verbosity=2)
