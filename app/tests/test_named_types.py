"""Nominal type identity, native execution, persistence, budgets and UI contracts."""
import copy,json,os,shutil,sys,unittest
from pathlib import Path
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import tool,test_integration as integration
import test_authoring as authoring
from cytools_core import CyToolError
ENUM={'kind':'enum','members':['Friendly','Hostile','Neutral'],'default':'Neutral','customMetadata':{'keep':True}}
FIELDS={'health':{'type':'float','default':100},'faction':{'type':'enum.Faction','default':'Neutral'}}
VALUE={'health':100,'faction':'Neutral'}
class NamedTypes(unittest.TestCase):
 @classmethod
 def setUpClass(cls):integration.EditorIntegration.setUpClass.__func__(cls)
 tearDownClass=classmethod(integration.EditorIntegration.tearDownClass.__func__)
 call=integration.EditorIntegration.call
 graph=authoring.NativeAuthoring.graph
 wire=authoring.NativeAuthoring.wire
 unlink=authoring.NativeAuthoring.unlink
 function=authoring.NativeAuthoring.function
 def node(self,g,type,**values):
  p={'graphId':g,'type':type,'position':[100,100]};p.update({key:values.pop(key) for key in ('typeRef','fieldRef','functionRef','classRef','methodRef','functionName') if key in values});n=self.call('graph_node_add',p)['node']['id']
  for key,value in values.items():self.call('graph_node_value',{'graphId':g,'nodeId':n,'key':key,'value':value,**({'functionName':p['functionName']} if 'functionName' in p else {})})
  return n
 def types(self,g):
  self.call('graph_type_define',{'graphId':g,'typeId':'enum.Faction','definition':ENUM});self.call('graph_type_define',{'graphId':g,'typeId':'struct.Stats','definition':{'kind':'struct','fields':FIELDS,'extension':{'preserve':9}}})
 def get(self,g):return self.call('graph_get',{'graphId':g})['graph']
 def test_01_enum_switch_equality_and_nominal_wire_identity(self):
  g=self.graph('EnumFlow');self.types(g);self.call('graph_type_define',{'graphId':g,'typeId':'enum.Other','definition':ENUM})
  self.unlink(g,self.get(g)['links'][0]['id']);literal=self.node(g,'typed.literal',typeRef='enum.Faction',value='Hostile');switch=self.node(g,'enum.switch',typeRef='enum.Faction');yes=self.node(g,'debug.print',text='Hostile');self.wire(g,'begin','then',switch,'exec');self.wire(g,literal,'value',switch,'value');self.wire(g,switch,'Hostile',yes,'exec');self.wire(g,switch,'Friendly','print','exec');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['Hostile'])
  other=self.node(g,'typed.literal',typeRef='enum.Other');equal=self.node(g,'enum.equal',typeRef='enum.Faction');before=self.get(g)
  with self.assertRaises(CyToolError):self.wire(g,other,'value',equal,'a')
  self.assertEqual(self.get(g),before)
  with self.assertRaises(CyToolError):self.call('graph_node_value',{'graphId':g,'nodeId':literal,'key':'value','value':'Unknown'})
 def test_02_struct_make_break_set_nested_value_semantics(self):
  g=self.graph('Records');self.types(g);self.call('graph_type_define',{'graphId':g,'typeId':'struct.Actor','definition':{'kind':'struct','fields':{'stats':{'type':'struct.Stats','default':VALUE},'name':{'type':'string','default':'Player'}}}})
  self.function(g,'Build',{'x':('struct.Stats',VALUE)},{'result':('struct.Stats',VALUE)})
  make=self.node(g,'struct.make',typeRef='struct.Stats',health=25,faction='Friendly',functionName='Build');edit=self.node(g,'struct.set',typeRef='struct.Stats',fieldRef='health',item=73,functionName='Build');self.wire(g,make,'value',edit,'value','Build');self.wire(g,edit,'value','return','result','Build')
  result=self.call('graph_function_evaluate',{'graphId':g,'name':'Build'})['values']['result'];self.assertEqual(result,{'health':73,'faction':'Friendly'});self.assertEqual(self.get(g)['functions']['Build']['nodes'][2]['values']['health'],25)
  self.call('graph_variable_add',{'graphId':g,'name':'Actor','type':'struct.Actor','default':{'stats':VALUE,'name':'Unit'}})
  before=self.get(g)
  for bad in ({'stats':VALUE},{'stats':VALUE,'name':'U','extra':1},{'stats':{'health':'bad','faction':'Neutral'},'name':'U'}):
   with self.assertRaises(CyToolError):self.call('graph_variable_default',{'graphId':g,'name':'Actor','value':bad})
  self.assertEqual(self.get(g),before)
 def test_03_named_arguments_macros_child_context_and_typed_variables(self):
  g=self.graph('TypedCalls');self.types(g);self.function(g,'Inline',{'x':('struct.Stats',VALUE)},{'result':('struct.Stats',VALUE)},kind='macro');self.wire(g,'arguments','x','return','result','Inline')
  self.function(g,'Wrapper',{'x':('struct.Stats',VALUE)},{'result':('struct.Stats',VALUE)});macro=self.node(g,'macro.call',functionRef='Inline',functionName='Wrapper');self.wire(g,'arguments','x',macro,'x','Wrapper');self.wire(g,macro,'result','return','result','Wrapper')
  payload={'health':177,'faction':'Hostile'};self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Wrapper','arguments':{'x':payload}})['values']['result'],payload)
  # An unconnected macro argument becomes a scoped named literal during expansion.
  root=self.node(g,'macro.call',functionRef='Inline',x=payload);split=self.node(g,'struct.break',typeRef='struct.Stats');self.wire(g,root,'result',split,'value');convert=self.node(g,'convert.float_string');self.wire(g,split,'health',convert,'value');self.wire(g,convert,'value','print','text');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['177'])
  self.call('graph_variable_add',{'graphId':g,'name':'Stats','type':'struct.Stats','default':VALUE});setter=self.node(g,'typed.set',typeRef='struct.Stats',value=payload);self.unlink(g,self.get(g)['links'][0]['id']);self.wire(g,'begin','then',setter,'exec');self.wire(g,setter,'then','print','exec');self.assertEqual(self.call('graph_run',{'graphId':g})['variables']['Stats'],payload)
  with self.assertRaises(CyToolError):self.call('graph_function_evaluate',{'graphId':g,'name':'Wrapper','arguments':{'x':{'health':1,'faction':'Other'}}})
 def test_04_class_fields_runtime_debug_edit_and_defaults_isolation(self):
  g=self.graph('TypedObjects');self.types(g);self.call('graph_class_define',{'graphId':g,'name':'Entity','fields':{'stats':{'type':'struct.Stats','default':VALUE}}})
  self.unlink(g,self.get(g)['links'][0]['id']);new=self.node(g,'object.new',classRef='Entity');self.wire(g,'begin','then',new,'exec');setter=self.node(g,'object.set.typed',typeRef='struct.Stats',field='stats',value={'health':22,'faction':'Friendly'});self.wire(g,new,'then',setter,'exec');self.wire(g,new,'object',setter,'object')
  self.call('graph_debug_start',{'graphId':g});state=self.call('graph_debug_continue',{'graphId':g});self.assertEqual(state['state'],'Completed',state);handle=next(iter(state['objects']));self.assertEqual(state['objects'][handle]['fields']['stats']['health'],22)
  edited=self.call('graph_debug_object_set',{'graphId':g,'object':handle,'field':'stats','value':{'health':8,'faction':'Hostile'}});self.assertEqual(edited['objects'][handle]['fields']['stats']['health'],8);self.assertEqual(self.get(g)['classes']['Entity']['fields']['stats']['default'],VALUE)
  with self.assertRaises(CyToolError):self.call('graph_debug_object_set',{'graphId':g,'object':handle,'field':'stats','value':{'health':1,'faction':'Missing'}})
 def test_05_declaration_validation_references_depth_and_undo(self):
  g=self.graph('TypeGuards');self.types(g);before=self.get(g);history=len(self.call('edit_history',{})['undo'])
  invalid=[('enum.Bad',{'kind':'enum','members':['X','X'],'default':'X'}),('enum.Bad',{'kind':'enum','members':['Bad name'],'default':'Bad name'}),('struct.Bad',{'kind':'struct','fields':{'self':{'type':'struct.Bad','default':{}}}}),('struct.Bad',{'kind':'struct','fields':{'missing':{'type':'struct.Missing','default':{}}}}),('struct.Bad',{'kind':'enum','members':['X'],'default':'X'})]
  for id,d in invalid:
   with self.assertRaises(CyToolError):self.call('graph_type_define',{'graphId':g,'typeId':id,'definition':d})
  with self.assertRaises(CyToolError):self.call('graph_type_remove',{'graphId':g,'typeId':'enum.Faction'})
  self.assertEqual(self.get(g),before);self.assertEqual(len(self.call('edit_history',{})['undo']),history)
  self.call('graph_type_define',{'graphId':g,'typeId':'enum.Unused','definition':ENUM});self.call('edit_undo',{});self.assertEqual(self.get(g),before);self.call('edit_redo',{});self.assertIn('enum.Unused',self.get(g)['types'])
  last='struct.Stats';value=VALUE
  for i in range(1,8):
   id='struct.N'+str(i);self.call('graph_type_define',{'graphId':g,'typeId':id,'definition':{'kind':'struct','fields':{'nested':{'type':last,'default':value}}}});last=id;value={'nested':value}
  with self.assertRaises(CyToolError):self.call('graph_type_define',{'graphId':g,'typeId':'struct.TooDeep','definition':{'kind':'struct','fields':{'nested':{'type':last,'default':value}}}})
  wide=['M'+str(i).zfill(2)+'x'*61 for i in range(64)];self.call('graph_type_define',{'graphId':g,'typeId':'enum.Wide','definition':{'kind':'enum','members':wide,'default':wide[0]}})
  before=self.get(g)
  with self.assertRaises(CyToolError):self.call('graph_type_define',{'graphId':g,'typeId':'struct.Amplified','definition':{'kind':'struct','fields':{'f'+str(i):{'type':'enum.Wide','default':wide[0]} for i in range(32)}}})
  self.assertEqual(self.get(g),before)
 def test_06_clipboard_cross_asset_types_conflicts_and_nominal_reroute(self):
  g=self.graph('CopyTypes');self.types(g);literal=self.node(g,'typed.literal',typeRef='struct.Stats');split=self.node(g,'struct.break',typeRef='struct.Stats');wire=self.wire(g,literal,'value',split,'value');self.call('graph_link_reroute',{'graphId':g,'linkId':wire['link']['id'],'position':[240,140]})
  ids=[n['id'] for n in self.get(g)['nodes'] if n['id'] not in ('begin','print')];fragment=self.call('graph_fragment_copy',{'graphId':g,'nodeIds':ids})['fragment'];self.assertEqual(fragment['types']['enum.Faction']['customMetadata'],{'keep':True})
  target=self.graph('PasteTypes');self.call('graph_fragment_paste',{'graphId':target,'fragment':fragment});self.assertEqual(self.get(target)['types'],self.get(g)['types'])
  self.call('graph_type_define',{'graphId':target,'typeId':'enum.Faction','definition':{**ENUM,'members':ENUM['members']+['Other']}});before=self.get(target)
  with self.assertRaises(CyToolError):self.call('graph_fragment_paste',{'graphId':target,'fragment':fragment})
  self.assertEqual(self.get(target),before)
 def test_07_type_changes_keep_existing_values_pins_valid_and_save_roundtrip(self):
  g=self.graph('PersistenceTypes');self.types(g);n=self.node(g,'typed.literal',typeRef='enum.Faction',value='Hostile');before=self.get(g)
  with self.assertRaises(CyToolError):self.call('graph_type_define',{'graphId':g,'typeId':'enum.Faction','definition':{**ENUM,'members':['Neutral'],'default':'Neutral'}})
  self.assertEqual(self.get(g),before);self.call('asset_save_graph',{'graphId':g});saved=json.loads((self.workspace/'data/assets'/self.get(g)['assetPath']).read_text('utf-8'));self.assertEqual(saved['graph']['types'],before['types'])
  description=self.call('graph_types_get',{'graphId':g});self.assertEqual(description['schemas']['enum.Faction']['enum'],ENUM['members']);self.assertFalse(description['schemas']['struct.Stats']['additionalProperties'])

 def test_08_named_function_fragment_into_another_function_and_enum_equality(self):
  g=self.graph('FunctionTransfer');self.types(g);self.function(g,'Echo',{'x':('struct.Stats',VALUE)},{'result':('struct.Stats',VALUE)});self.wire(g,'arguments','x','return','result','Echo');call=self.node(g,'function.call',functionRef='Echo');fragment=self.call('graph_fragment_copy',{'graphId':g,'nodeIds':[call]})['fragment']
  target=self.graph('FunctionPaste');self.function(target,'Caller',{'x':('float',0)},{'result':('float',0)});self.call('graph_fragment_paste',{'graphId':target,'functionName':'Caller','fragment':fragment});owner=self.get(target);self.assertIn('struct.Stats',owner['types']);self.assertIn('Echo',owner['functions']);self.assertNotIn('types',owner['functions']['Caller']);self.assertEqual(self.call('graph_function_evaluate',{'graphId':target,'name':'Echo'})['values']['result'],VALUE)
  self.function(g,'IsHostile',{'x':('enum.Faction','Neutral')},{'result':('bool',False)});eq=self.node(g,'enum.equal',typeRef='enum.Faction',b='Hostile',functionName='IsHostile');self.wire(g,'arguments','x',eq,'a','IsHostile');self.wire(g,eq,'value','return','result','IsHostile')
  for value,expected in [('Friendly',False),('Hostile',True)]:self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'IsHostile','arguments':{'x':value}})['values']['result'],expected)

 def test_09_actual_type_declaration_and_record_property_gestures(self):
  if not self.gui:self.skipTest('Graphical host required')
  import subprocess
  root=self.workspace/'types-gestures';(root/'app').mkdir(parents=True);shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copyfile(APP/'interface_features.json',root/'app/interface_features.json');shutil.copytree(APP/'locales',root/'app/locales')
  report=ROOT/'data/reports/types-gestures.json';report.unlink(missing_ok=True)
  run=subprocess.run([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Scripting','--test-types',str(report),'--exit-after','20'],timeout=35)
  fatal=root/'data/logs/native-fatal.json'
  if fatal.exists():shutil.copyfile(fatal,ROOT/'data/reports/types-gesture-fatal.json')
  self.assertEqual(run.returncode,0,report.read_text('utf-8') if report.exists() else fatal.read_text('utf-8') if fatal.exists() else 'No gesture report');self.assertTrue(json.loads(report.read_text('utf-8'))['passed']);self.assertFalse((root/'data/settings/ui-preferences.json').exists());self.assertFalse((root/'data/settings/editor.ini').exists())

 def test_10_execution_child_calls_pure_named_function_and_debugger_returns(self):
  g=self.graph('RuntimeContext');self.types(g);self.function(g,'Echo',{'x':('struct.Stats',VALUE)},{'result':('struct.Stats',VALUE)});self.wire(g,'arguments','x','return','result','Echo')
  self.function(g,'Flow',{'x':('struct.Stats',VALUE)},{'result':('struct.Stats',VALUE)},mode='execution');echo=self.node(g,'function.call',functionRef='Echo',functionName='Flow');self.wire(g,'arguments','x',echo,'x','Flow');self.wire(g,echo,'result','return','result','Flow');self.wire(g,'arguments','then','return','exec','Flow')
  payload={'health':61,'faction':'Hostile'};call=self.node(g,'function.call',functionRef='Flow',x=payload);split=self.node(g,'struct.break',typeRef='struct.Stats');text=self.node(g,'convert.float_string');self.wire(g,call,'result',split,'value');self.wire(g,split,'health',text,'value');self.wire(g,text,'value','print','text');self.unlink(g,self.get(g)['links'][0]['id']);self.wire(g,'begin','then',call,'exec');self.wire(g,call,'then','print','exec');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['61'])
  self.call('graph_debug_start',{'graphId':g});state=self.call('graph_debug_continue',{'graphId':g});self.assertEqual(state['state'],'Completed',state);self.assertEqual(state['messages'],['61'])
  self.call('graph_type_define',{'graphId':g,'typeId':'enum.Faction','definition':{**ENUM,'members':ENUM['members']+['NewMember']}});self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Stale')

if __name__=='__main__':unittest.main(verbosity=2)
