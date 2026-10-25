"""Explicit nominal enum/record shape migration through the native owner."""
import copy,json,os,shutil,subprocess,sys,tempfile,time,unittest
from pathlib import Path
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import tool,test_integration as integration,test_authoring as authoring,test_graph_interfaces as interfaces
from cytools_core import CyToolError

def p(t='float',v=1):return {'type':t,'default':v}
TEAM={'kind':'enum','members':['A','B','C'],'default':'A','metadata':{'keep':True}}
FIELDS={'health':{**p(),'hint':'keep'},'team':p('enum.Team','A')}
VALUE={'health':9,'team':'B'}
class TypeMigrations(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  integration.EditorIntegration.setUpClass.__func__(cls)
  # The descriptor may exist while its initial write handle is still open.
  deadline=time.monotonic()+4
  while True:
   try:tool.editor_call('editor_status',{},timeout=1);break
   except CyToolError:
    if cls.native.poll() is not None or time.monotonic()>=deadline:raise
    time.sleep(.02)
 tearDownClass=classmethod(integration.EditorIntegration.tearDownClass.__func__)
 call=integration.EditorIntegration.call;graph=authoring.NativeAuthoring.graph;wire=authoring.NativeAuthoring.wire;unlink=authoring.NativeAuthoring.unlink;function=authoring.NativeAuthoring.function
 get=interfaces.Interfaces.get;cls=interfaces.Interfaces.cls;output=interfaces.Interfaces.output
 def node(self,g,type,**values):
  params={'graphId':g,'type':type,'position':[100,100]};params.update({k:values.pop(k) for k in ('interfaceRef','methodRef','classRef','functionRef','functionName','typeRef','fieldRef','variableName') if k in values});n=self.call('graph_node_add',params)['node']['id']
  for k,v in values.items():self.call('graph_node_value',{'graphId':g,'nodeId':n,'key':k,'value':v,**({'functionName':params['functionName']} if 'functionName' in params else {})})
  return n
 def types(self,g):
  self.call('graph_type_define',{'graphId':g,'typeId':'enum.Team','definition':TEAM});self.call('graph_type_define',{'graphId':g,'typeId':'struct.Stats','definition':{'kind':'struct','fields':FIELDS,'metadata':{'preserve':7}}})
 def simple(self,name='TypeMigration'):
  g=self.graph(name);self.types(g);self.call('graph_variable_add',{'graphId':g,'name':'Record','type':'struct.Stats','default':VALUE});get=self.node(g,'typed.get',typeRef='struct.Stats',variableName='Record');split=self.node(g,'struct.break',typeRef='struct.Stats');self.wire(g,get,'value',split,'value');self.output(g,split,'health');return g,split
 def request(self,g,**kw):
  fields=copy.deepcopy(FIELDS);fields['hp']=fields.pop('health');fields['armor']=p(v=5)
  return {'graphId':g,'typeId':'struct.Stats','definition':{'kind':'struct','fields':fields},'renames':{'health':'hp'},**kw}
 def plan(self,r):return self.call('graph_type_migration_plan',r)
 def apply(self,r,plan,**kw):return self.call('graph_type_migration_apply',{**r,'planHash':plan['planHash'],**kw})
 def test_01_structure_rename_add_preserves_data_wires_metadata_and_one_undo(self):
  g,split=self.simple();r=self.request(g);before=self.get(g);revision=self.call('editor_status')['revision'];history=self.call('edit_history');plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.assertFalse(plan['requiresLossAcceptance']);self.assertEqual(self.get(g),before);self.assertEqual(self.call('editor_status')['revision'],revision);self.assertEqual(self.call('edit_history'),history);self.apply(r,plan);after=self.get(g);self.assertEqual(after['variables']['Record']['default'],{'hp':9,'team':'B','armor':5});self.assertEqual(after['types']['struct.Stats']['fields']['hp']['hint'],'keep');self.assertEqual(after['types']['struct.Stats']['metadata'],{'preserve':7});self.assertEqual({l['id'] for l in before['links']},{l['id'] for l in after['links']});self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9']);self.call('edit_undo');self.assertEqual(self.get(g),before);self.call('edit_redo');self.assertEqual(self.get(g),after)
 def test_02_enum_member_rename_updates_nested_values_and_switch_branches(self):
  g,split=self.simple('EnumRename');self.unlink(g,'start');switch=self.node(g,'enum.switch',typeRef='enum.Team');self.wire(g,'begin','then',switch,'exec');self.wire(g,split,'team',switch,'value');self.wire(g,switch,'B','print','exec');r={'graphId':g,'typeId':'enum.Team','definition':{'kind':'enum','members':['A','Enemy','C'],'default':'A'},'renames':{'B':'Enemy'}};before=self.get(g);plan=self.plan(r);self.assertFalse(plan['requiresLossAcceptance'],plan);self.apply(r,plan);after=self.get(g);self.assertEqual(after['variables']['Record']['default']['team'],'Enemy');self.assertEqual({l['id'] for l in before['links']},{l['id'] for l in after['links']});self.assertTrue(any(l['fromNode']==switch and l['fromPin']=='Enemy' for l in after['links']));self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9'])
 def test_03_removed_enum_resets_values_and_reports_dropped_branch(self):
  g,split=self.simple('EnumLoss');self.unlink(g,'start');switch=self.node(g,'enum.switch',typeRef='enum.Team');self.wire(g,'begin','then',switch,'exec');self.wire(g,split,'team',switch,'value');self.wire(g,switch,'B','print','exec');r={'graphId':g,'typeId':'enum.Team','definition':{'kind':'enum','members':['A','C'],'default':'A'}};plan=self.plan(r);self.assertTrue(plan['requiresLossAcceptance']);self.assertEqual(plan['summary']['linksRemoved'],1);before=self.get(g)
  with self.assertRaises(CyToolError):self.apply(r,plan)
  self.assertEqual(self.get(g),before);self.apply(r,plan,acceptLoss=True);self.assertEqual(self.get(g)['variables']['Record']['default']['team'],'A');self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],[]);self.call('edit_undo');self.assertEqual(self.get(g),before)
 def test_04_removed_record_fields_and_type_changes_are_explicit_losses(self):
  g,split=self.simple('FieldLoss');r=self.request(g,definition={'kind':'struct','fields':{'team':p('enum.Team','A')}},renames={});plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.assertTrue(plan['requiresLossAcceptance']);self.assertEqual(plan['summary']['linksRemoved'],1);self.apply(r,plan,acceptLoss=True);self.assertEqual(self.get(g)['variables']['Record']['default'],{'team':'B'});self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['0'])
  g,split=self.simple('FieldTypeLoss');fields=copy.deepcopy(FIELDS);fields['health']=p('int',3);r=self.request(g,definition={'kind':'struct','fields':fields},renames={});plan=self.plan(r);self.assertTrue(plan['requiresLossAcceptance']);self.assertGreater(plan['summary']['valuesReset'],0);self.apply(r,plan,acceptLoss=True);self.assertEqual(self.get(g)['variables']['Record']['default']['health'],3)
 def test_05_parent_type_defaults_and_deep_values_are_normalized_once(self):
  g,split=self.simple('NestedTypeMigration');self.call('graph_type_define',{'graphId':g,'typeId':'struct.Parent','definition':{'kind':'struct','fields':{'stats':p('struct.Stats',VALUE)}}});self.call('graph_type_define',{'graphId':g,'typeId':'struct.Root','definition':{'kind':'struct','fields':{'parent':p('struct.Parent',{'stats':VALUE})}}});self.call('graph_variable_add',{'graphId':g,'name':'Nested','type':'struct.Root','default':{'parent':{'stats':VALUE}}});r=self.request(g);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);out=self.get(g);expected={'hp':9,'armor':5,'team':'B'};self.assertEqual(out['types']['struct.Parent']['fields']['stats']['default'],expected);self.assertEqual(out['types']['struct.Root']['fields']['parent']['default'],{'stats':expected});self.assertEqual(out['variables']['Nested']['default'],{'parent':{'stats':expected}})
  r={'graphId':g,'typeId':'enum.Team','definition':{'kind':'enum','members':['A','B','C'],'default':'A'},'renames':{'A':'B','B':'A'}};plan=self.plan(r);self.apply(r,plan);out=self.get(g);self.assertEqual(out['variables']['Nested']['default']['parent']['stats']['team'],'A');self.assertEqual(out['types']['struct.Parent']['fields']['stats']['default']['team'],'A')
 def test_06_function_macro_class_interface_defaults_and_calls_migrate_together(self):
  g,split=self.simple('DeclarationTypes');self.function(g,'Echo',{'data':('struct.Stats',VALUE)},{'result':('struct.Stats',VALUE)});self.wire(g,'arguments','data','return','result','Echo');self.node(g,'function.call',functionRef='Echo',data=VALUE);self.function(g,'Inline',{'data':('struct.Stats',VALUE)},{'result':('struct.Stats',VALUE)},kind='macro');self.wire(g,'arguments','data','return','result','Inline');self.node(g,'macro.call',functionRef='Inline',data=VALUE)
  self.call('graph_interface_define',{'graphId':g,'name':'Readable','definition':{'methods':{'Read':{'mode':'pure','inputs':{'data':p('struct.Stats',VALUE)},'outputs':{'result':p('struct.Stats',VALUE)}}}}});self.function(g,'Impl',{'self':('object',''),'data':('struct.Stats',VALUE)},{'result':('struct.Stats',VALUE)});self.wire(g,'arguments','data','return','result','Impl');self.cls(g,'RecordClass',{'Read':'Impl'},['Readable'],fields={'stats':p('struct.Stats',VALUE)})
  self.node(g,'interface.call',interfaceRef='Readable',methodRef='Read',data=VALUE);self.node(g,'object.call',classRef='RecordClass',methodRef='Read',data=VALUE);self.function(g,'Work',{}, {},mode='execution');self.call('graph_variable_add',{'graphId':g,'functionName':'Work','name':'Local','type':'struct.Stats','default':VALUE})
  r=self.request(g);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);out=self.get(g);expected={'hp':9,'team':'B','armor':5};self.assertEqual(out['functions']['Work']['variables']['Local']['default'],expected);self.assertTrue(all(n['values']['data']==expected for n in out['nodes'] if n['type'] in ('function.call','macro.call','interface.call','object.call')));self.assertEqual(out['functions']['Echo']['inputs']['data']['default'],expected);self.assertEqual(out['classes']['RecordClass']['fields']['stats']['default'],expected);self.assertEqual(out['interfaces']['Readable']['methods']['Read']['inputs']['data']['default'],expected);self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Echo'})['values']['result'],expected);self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Inline'})['values']['result'],expected);self.assertNotIn('types',out['functions']['Echo'])
 def test_07_make_break_set_references_and_removed_set_block(self):
  g,split=self.simple('SetFieldMigration');make=self.node(g,'struct.make',typeRef='struct.Stats',health=22,team='B');setfield=self.node(g,'struct.set',typeRef='struct.Stats',fieldRef='health',item=13);self.wire(g,make,'value',setfield,'value');old=next(l for l in self.get(g)['links'] if l['toNode']==split and l['toPin']=='value');self.unlink(g,old['id']);self.wire(g,setfield,'value',split,'value');r=self.request(g);plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);out=self.get(g);self.assertEqual(next(n for n in out['nodes'] if n['id']==setfield)['field'],'hp');self.assertEqual(next(n for n in out['nodes'] if n['id']==make)['values']['hp'],22);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['13']);r=self.request(g,definition={'kind':'struct','fields':{'team':p('enum.Team','A')}},renames={});before=self.get(g);plan=self.plan(r);self.assertFalse(plan['valid'])
  with self.assertRaises(CyToolError):self.apply(r,plan,acceptLoss=True)
  self.assertEqual(self.get(g),before)
 def test_08_nominal_field_type_replacement_does_not_cast_equal_enum_strings(self):
  g,split=self.simple('NominalReplacement');self.call('graph_type_define',{'graphId':g,'typeId':'enum.Other','definition':{**TEAM,'default':'C'}});fields=copy.deepcopy(FIELDS);fields['team']=p('enum.Other','C');r=self.request(g,definition={'kind':'struct','fields':fields},renames={});plan=self.plan(r);self.assertTrue(plan['requiresLossAcceptance']);self.apply(r,plan,acceptLoss=True);self.assertEqual(self.get(g)['variables']['Record']['default']['team'],'C')
 def test_09_invalid_rename_kind_cycle_and_stale_requests_preserve_graph(self):
  g,split=self.simple('TypeGuards');before=self.get(g)
  for request in [self.request(g,renames={'absent':'hp'}),self.request(g,renames={'health':'missing'}),self.request(g,renames={'health':'hp','team':'hp'}),self.request(g,definition={'kind':'enum','members':['A'],'default':'A'}),self.request(g,definition={'kind':'struct','fields':{'self':p('struct.Stats',VALUE)}},renames={})]:
   with self.assertRaises(CyToolError):self.plan(request)
  self.assertEqual(self.get(g),before);r=self.request(g);plan=self.plan(r)
  with self.assertRaises(CyToolError):self.apply({**r,'renames':{}},plan,acceptLoss=True)
  self.call('graph_node_comment',{'graphId':g,'nodeId':split,'text':'stale'});before=self.get(g)
  with self.assertRaises(CyToolError):self.apply(r,plan)
  self.assertEqual(self.get(g),before)
 def test_10_bounded_expansion_rejects_oversized_parent_defaults(self):
  g,split=self.simple('TypeSize');value={'health':1,'team':'A'};self.call('graph_type_define',{'graphId':g,'typeId':'struct.Large','definition':{'kind':'struct','fields':{'slot'+str(i):p('struct.Stats',value) for i in range(20)}}});fields=copy.deepcopy(FIELDS);fields['large']=p('string','x'*4096);r=self.request(g,definition={'kind':'struct','fields':fields},renames={});before=self.get(g);plan=self.plan(r);self.assertFalse(plan['valid'],plan)
  with self.assertRaises(CyToolError):self.apply(r,plan,acceptLoss=True)
  self.assertEqual(self.get(g),before)
 def test_11_noop_default_change_persistence_and_official_core_debug_stale(self):
  g,split=self.simple('TypeCore');r={'graphId':g,'typeId':'struct.Stats','definition':self.get(g)['types']['struct.Stats']};history=self.call('edit_history');self.apply(r,self.plan(r));self.assertEqual(self.call('edit_history'),history);fields=copy.deepcopy(FIELDS);fields['health']['default']=100;r=self.request(g,definition={'kind':'struct','fields':fields},renames={});self.apply(r,self.plan(r));self.assertEqual(self.get(g)['variables']['Record']['default']['health'],9)
  self.call('graph_debug_start',{'graphId':g});r=self.request(g)
  with tool.create_runtime(self.workspace/'type-migration-core') as runtime:
   actor=runtime.authenticate(runtime.create_client('Type migrations')['token']);session=runtime.create_session(actor)
   def run(op,params):
    job=runtime.wait(actor,runtime.submit(actor,session['sessionId'],op,params)['jobId']);self.assertEqual(job['state'],'Completed',job);return job['outputs']
   plan=run('graph_type_migration_plan',r);self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Paused');run('graph_type_migration_apply',{**r,'planHash':plan['planHash']});runtime.finish_task(actor,session['sessionId'])
  self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Stale');self.call('asset_save_graph',{'graphId':g});path=self.get(g)['assetPath'];self.call('asset_open',{'path':path});self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9'])
 def test_12_native_type_migration_apply_and_loss_gestures_en_fr(self):
  if not self.gui:self.skipTest('Graphical host required')
  for lang in ('en','fr'):
   with tempfile.TemporaryDirectory(prefix='type-migration-ui-',dir=ROOT/'data/reports') as tmp:
    root=Path(tmp);(root/'app').mkdir();shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copytree(APP/'locales',root/'app/locales');report=root/'report.json';process=subprocess.run([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Scripting','--lang',lang,'--test-type-migration',str(report),'--exit-after','45'],timeout=60);self.assertEqual(process.returncode,0);self.assertTrue(report.exists());result=json.loads(report.read_text('utf-8'));self.assertTrue(result['passed'],result);self.assertEqual(len(result['checks']),12);shutil.copyfile(report,ROOT/'data/reports'/('type-migrations-gestures-'+lang+'.json'));self.assertFalse((root/'data/settings/ui-preferences.json').exists());self.assertFalse((root/'data/settings/editor.ini').exists())

 def test_13_optional_variable_sections_stay_absent_on_noop(self):
  g=self.graph('OptionalVariables');self.types(g);self.function(g,'Empty',{}, {'result':('float',0)});source=self.get(g);source.pop('assetPath',None);source.pop('variables',None);source['functions']['Empty'].pop('variables',None)
  path=self.workspace/'data/assets/optional.cyasset';path.write_text(json.dumps({'format':'cygame-graph','schemaVersion':1,'graph':source}),'utf-8');owner=self.call('asset_open',{'path':'optional.cyasset'})['graphId'];before=self.get(owner);r={'graphId':owner,'typeId':'struct.Stats','definition':before['types']['struct.Stats']};history=self.call('edit_history');plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.apply(r,plan);self.assertEqual(self.get(owner),before);self.assertEqual(self.call('edit_history'),history);self.assertNotIn('variables',before);self.assertNotIn('variables',before['functions']['Empty'])

 def test_14_bounded_details_keep_exact_loss_counts(self):
  g=self.graph('ManyTypeValues');self.types(g);source=self.get(g);source.pop('assetPath',None);source['links']=[];source['nodes']=[{'id':'literal'+str(i),'type':'typed.literal','typeRef':'enum.Team','position':[0,0],'values':{'value':'B'}} for i in range(300)]
  path=self.workspace/'data/assets/many-types.cyasset';path.write_text(json.dumps({'format':'cygame-graph','schemaVersion':1,'graph':source}),'utf-8');owner=self.call('asset_open',{'path':'many-types.cyasset'})['graphId'];r={'graphId':owner,'typeId':'enum.Team','definition':{'kind':'enum','members':['A','C'],'default':'A'}};plan=self.plan(r);self.assertTrue(plan['valid'],plan);self.assertTrue(plan['truncated']);self.assertGreater(plan['changeCount'],512);self.assertLessEqual(len(plan['changes']),512);self.assertLessEqual(sum(len(json.dumps(c,separators=(',',':'),ensure_ascii=False).encode()) for c in plan['changes']),256*1024);self.assertEqual(plan['summary']['valuesReset'],300);self.apply(r,plan,acceptLoss=True);self.assertTrue(all(n['values']['value']=='A' for n in self.get(owner)['nodes']))

if __name__=='__main__':unittest.main(verbosity=2)
