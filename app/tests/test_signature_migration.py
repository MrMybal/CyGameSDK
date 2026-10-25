"""Explicit, hash-guarded, whole-owner graph signature migrations."""
import copy,json,os,shutil,subprocess,sys,tempfile,unittest
from pathlib import Path
APP=Path(__file__).resolve().parents[1];ROOT=APP.parent
sys.path.insert(0,str(APP));sys.path.insert(0,str(APP/'tests'))
import tool,test_integration as integration,test_authoring as authoring,test_graph_interfaces as interfaces
from cytools_core import CyToolError

def port(t='float',v=0):return {'type':t,'default':v}
class Migrations(unittest.TestCase):
 @classmethod
 def setUpClass(cls):integration.EditorIntegration.setUpClass.__func__(cls)
 tearDownClass=classmethod(integration.EditorIntegration.tearDownClass.__func__)
 call=integration.EditorIntegration.call;graph=authoring.NativeAuthoring.graph;wire=authoring.NativeAuthoring.wire;unlink=authoring.NativeAuthoring.unlink;function=authoring.NativeAuthoring.function
 get=interfaces.Interfaces.get;node=interfaces.Interfaces.node;define=interfaces.Interfaces.define;cls=interfaces.Interfaces.cls;method=interfaces.Interfaces.method;output=interfaces.Interfaces.output;flow=interfaces.Interfaces.flow
 def simple(self,name='Migration',kind='function'):
  g=self.graph(name);self.function(g,'Echo',{'x':('float',2)},{'result':('float',0)},kind=kind);self.wire(g,'arguments','x','return','result','Echo');n=self.node(g,'macro.call' if kind=='macro' else 'function.call',functionRef='Echo',x=9);self.output(g,n);return g,n
 def request(self,g,**kw):return {'graphId':g,'target':'function','name':'Echo','inputs':{'data':port(v=3)},'outputs':{'answer':port()},'renames':{'inputs':{'x':'data'},'outputs':{'result':'answer'}},**kw}
 def plan(self,p):return self.call('graph_signature_plan',p)
 def apply(self,p,plan,**kw):return self.call('graph_signature_apply',{**p,'planHash':plan['planHash'],**kw})
 def test_01_lossless_rename_preserves_wires_ids_values_readonly_preview_and_undo(self):
  g,n=self.simple();p=self.request(g);before=self.get(g);rev=self.call('editor_status')['revision'];history=self.call('edit_history');plan=self.plan(p)
  self.assertTrue(plan['valid'],plan);self.assertFalse(plan['requiresLossAcceptance']);self.assertGreaterEqual(plan['summary']['linksRenamed'],2);self.assertEqual(self.get(g),before);self.assertEqual(self.call('editor_status')['revision'],rev);self.assertEqual(self.call('edit_history'),history)
  self.apply(p,plan);after=self.get(g);self.assertEqual({l['id'] for l in after['links']},{l['id'] for l in before['links']});self.assertEqual(next(v for v in after['nodes'] if v['id']==n)['values']['data'],9);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9']);self.call('edit_undo');self.assertEqual(self.get(g),before);self.call('edit_redo');self.assertEqual(self.get(g),after)
 def test_02_added_ports_defaults_existing_fallbacks_and_idempotent_noop(self):
  g,n=self.simple('MigrationAdd');p=self.request(g,inputs={'x':port(v=17),'bonus':port(v=7)},outputs={'result':port(v=11)},renames={});plan=self.plan(p);self.assertFalse(plan['requiresLossAcceptance']);self.apply(p,plan);graph=self.get(g);values=next(v for v in graph['nodes'] if v['id']==n)['values'];self.assertEqual(values,{'x':9,'bonus':7});self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9']);history=self.call('edit_history');self.apply(p,self.plan(p));self.assertEqual(self.call('edit_history'),history)
 def test_03_removed_ports_require_explicit_loss_acceptance_and_preserve_source_until_apply(self):
  g,n=self.simple('MigrationRemove');p=self.request(g,inputs={},outputs={'result':port()},renames={});plan=self.plan(p);self.assertTrue(plan['requiresLossAcceptance']);self.assertEqual(plan['summary']['linksRemoved'],1);self.assertEqual(plan['summary']['valuesRemoved'],1);before=self.get(g)
  with self.assertRaises(CyToolError):self.apply(p,plan)
  self.assertEqual(self.get(g),before);self.apply(p,plan,acceptLoss=True);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['0']);self.call('edit_undo');self.assertEqual(self.get(g),before)
 def test_04_type_change_resets_fallback_and_drops_incompatible_links(self):
  g,n=self.simple('MigrationType');p=self.request(g,inputs={'x':port('bool',False)},outputs={'result':port()},renames={});plan=self.plan(p);self.assertTrue(plan['valid']);self.assertEqual(plan['summary']['valuesReset'],1);self.assertEqual(plan['summary']['linksRemoved'],1);self.apply(p,plan,acceptLoss=True);self.assertIs(next(v for v in self.get(g)['nodes'] if v['id']==n)['values']['x'],False)
 def test_05_preview_hash_rejects_source_edits_and_changed_request(self):
  g,n=self.simple('MigrationStale');p=self.request(g);plan=self.plan(p);before=self.get(g)
  with self.assertRaises(CyToolError):self.apply({**p,'inputs':{'data':port(v=4)}},plan)
  self.assertEqual(self.get(g),before);self.call('graph_node_comment',{'graphId':g,'nodeId':n,'text':'Changed after preview'});before=self.get(g)
  with self.assertRaises(CyToolError):self.apply(p,plan)
  self.assertEqual(self.get(g),before);self.apply(p,self.plan(p))
 def test_06_nested_macro_and_local_context_persistence(self):
  g,n=self.simple('MigrationMacro','macro');self.function(g,'Wrap',{'x':('float',0)},{'result':('float',0)});macro=self.node(g,'macro.call',functionRef='Echo',functionName='Wrap',x=13);self.wire(g,macro,'result','return','result','Wrap');p=self.request(g);self.apply(p,self.plan(p));self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Wrap'})['values']['result'],13);self.assertNotIn('functions',self.get(g)['functions']['Echo']);self.call('asset_save_graph',{'graphId':g});path=self.get(g)['assetPath'];self.call('asset_open',{'path':path});self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['9'])
 def test_07_interface_closure_updates_overrides_aliases_and_unrelated_classes(self):
  g=self.graph('MigrationInterfaces');self.define(g);self.define(g,'Other');self.method(g,'BaseImpl',10);self.method(g,'DerivedImpl',100);self.cls(g,'Base',{'Read':'BaseImpl'},['Readable','Other']);self.cls(g,'Derived',{'Read':'DerivedImpl'},parent='Base');self.cls(g,'Unrelated',{'Read':'BaseImpl'},['Readable']);obj,call=self.flow(g,cls='Derived');before=self.get(g)
  p={'graphId':g,'target':'interface','name':'Readable','method':'Read','inputs':{'bonus':port(v=11)},'outputs':{'score':port()},'renames':{'inputs':{'amount':'bonus'},'outputs':{'result':'score'}}};plan=self.plan(p);self.assertTrue(plan['valid'],plan);self.assertFalse(plan['requiresLossAcceptance']);self.assertEqual(set(plan['targets']['functions']),{'BaseImpl','DerivedImpl'});self.assertEqual(len(plan['targets']['interfaces']),2);self.apply(p,plan);after=self.get(g);self.assertEqual(after['interfaces']['Readable']['methods']['Read']['inputs']['bonus']['default'],11);self.assertEqual(after['interfaces']['Other']['methods']['Read']['inputs']['bonus']['default'],7);self.assertEqual(after['functions']['BaseImpl']['inputs']['bonus']['default'],99);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],['107']);self.call('edit_undo');self.assertEqual(self.get(g),before)
 def test_08_invalid_maps_self_and_foreach_contract_are_non_mutating(self):
  g,n=self.simple('MigrationInvalid');before=self.get(g)
  for renames in ({'inputs':{'missing':'data'}},{'inputs':{'x':'absent'}},{'unknown':{}},{'outputs':{'result':'absent'}}):
   with self.assertRaises(CyToolError):self.plan(self.request(g,renames=renames))
  self.assertEqual(self.get(g),before)
  self.method(g,'Method');self.cls(g,'Class',{'Read':'Method'});before=self.get(g)
  with self.assertRaises(CyToolError):self.plan({'graphId':g,'target':'function','name':'Method','inputs':{'amount':port()},'outputs':{'result':port()}})
  self.assertEqual(self.get(g),before)
  self.function(g,'Each',{'item':('float',0),'index':('int',0)},{},mode='execution');self.wire(g,'arguments','then','return','exec','Each');self.node(g,'flow.foreach',functionRef='Each');p={'graphId':g,'target':'function','name':'Each','inputs':{'value':port(),'index':port('int',0)},'outputs':{},'renames':{'inputs':{'item':'value'}}};plan=self.plan(p);self.assertFalse(plan['valid']);before=self.get(g)
  with self.assertRaises(CyToolError):self.apply(p,plan,acceptLoss=True)
  self.assertEqual(self.get(g),before)
 def test_09_material_function_and_named_nominal_types(self):
  g=self.call('asset_new_graph',{'name':'MigrationMaterial','domain':'material'})['graphId'];self.function(g,'Echo',{'x':('float',2)},{'result':('float',0)});self.wire(g,'arguments','x','return','result','Echo');p=self.request(g);self.apply(p,self.plan(p));self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Echo','arguments':{'data':4}})['values']['answer'],4)
  g=self.graph('MigrationNominal');self.call('graph_type_define',{'graphId':g,'typeId':'enum.First','definition':{'kind':'enum','members':['A'],'default':'A'}});self.call('graph_type_define',{'graphId':g,'typeId':'enum.Second','definition':{'kind':'enum','members':['A'],'default':'A'}});self.function(g,'Echo',{'x':('enum.First','A')},{'result':('enum.First','A')});self.wire(g,'arguments','x','return','result','Echo');self.node(g,'function.call',functionRef='Echo');p=self.request(g,inputs={'x':port('enum.Second','A')},outputs={'result':port('enum.First','A')},renames={});plan=self.plan(p);self.assertEqual(plan['summary']['linksRemoved'],1);self.assertEqual(plan['summary']['valuesReset'],1)
 def test_10_official_core_preview_apply_and_debugger_invalidation(self):
  g,n=self.simple('MigrationCore');p=self.request(g);self.call('graph_debug_start',{'graphId':g})
  with tool.create_runtime(self.workspace/'migration-core') as runtime:
   actor=runtime.authenticate(runtime.create_client('Migrations')['token']);session=runtime.create_session(actor)
   def run(op,params):return runtime.wait(actor,runtime.submit(actor,session['sessionId'],op,params)['jobId'])
   job=run('graph_signature_plan',p);self.assertEqual(job['state'],'Completed',job);plan=job['outputs'];self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Paused');job=run('graph_signature_apply',{**p,'planHash':plan['planHash']});self.assertEqual(job['state'],'Completed',job);runtime.finish_task(actor,session['sessionId'])
  self.assertEqual(self.call('graph_debug_status',{'graphId':g})['state'],'Stale')

 def test_11_native_preview_apply_loss_acceptance_and_keyboard_undo_en_fr(self):
  if not self.gui:self.skipTest('Graphical host required')
  for lang in ('en','fr'):
   with tempfile.TemporaryDirectory(prefix='migration-ui-',dir=ROOT/'data/reports') as tmp:
    root=Path(tmp);(root/'app').mkdir();shutil.copyfile(APP/'CyTool.json',root/'app/CyTool.json');shutil.copytree(APP/'locales',root/'app/locales');report=root/'report.json'
    process=subprocess.run([str(ROOT/'CyGameEditor.exe'),'--root',str(root),'--workspace','Scripting','--lang',lang,'--test-migration',str(report),'--exit-after','45'],timeout=60)
    self.assertEqual(process.returncode,0);self.assertTrue(report.exists());result=json.loads(report.read_text('utf-8'));self.assertTrue(result['passed'],result);self.assertEqual(len(result['checks']),12);shutil.copyfile(report,ROOT/'data/reports'/('migrations-gestures-'+lang+'.json'));self.assertFalse((root/'data/settings/ui-preferences.json').exists());self.assertFalse((root/'data/settings/editor.ini').exists())
 def test_12_callback_extra_argument_migration_and_swap_collision(self):
  g=self.graph('MigrationCallback');self.function(g,'Each',{'item':('float',0),'index':('int',0),'extra':('float',3)},{},mode='execution');text=self.node(g,'convert.float_string',functionName='Each');out=self.node(g,'debug.print',functionName='Each');self.wire(g,'arguments','extra',text,'value','Each');self.wire(g,text,'value',out,'text','Each');self.wire(g,'arguments','then',out,'exec','Each');self.wire(g,out,'then','return','exec','Each');loop=self.node(g,'flow.foreach',functionRef='Each',values=[2],extra=14);self.unlink(g,'start');self.wire(g,'begin','then',loop,'exec');self.wire(g,loop,'then','print','exec');before=self.call('graph_run',{'graphId':g})['messages']
  p={'graphId':g,'target':'function','name':'Each','inputs':{'item':port(),'index':port('int',0),'bonus':port(v=3)},'outputs':{},'renames':{'inputs':{'extra':'bonus'}}};plan=self.plan(p);self.assertTrue(plan['valid'],plan);self.assertFalse(plan['requiresLossAcceptance']);self.apply(p,plan);self.assertEqual(self.call('graph_run',{'graphId':g})['messages'],before)
  g=self.graph('MigrationSwap');self.function(g,'Echo',{'a':('float',1),'b':('float',2)},{'result':('float',0)});self.wire(g,'arguments','a','return','result','Echo');self.node(g,'function.call',functionRef='Echo',a=9,b=4);p={'graphId':g,'target':'function','name':'Echo','inputs':{'a':port(v=1),'b':port(v=2)},'outputs':{'result':port()},'renames':{'inputs':{'a':'b','b':'a'}}};plan=self.plan(p);self.assertFalse(plan['requiresLossAcceptance']);self.apply(p,plan);self.assertEqual(self.call('graph_function_evaluate',{'graphId':g,'name':'Echo','arguments':{'b':8}})['values']['result'],8)
  with self.assertRaises(CyToolError):self.plan({**p,'renames':{'inputs':{'a':'b'}}})
 def test_13_bounded_details_keep_complete_loss_counts_and_metadata(self):
  g=self.graph('MigrationBounded');graph=self.get(g);self.function(g,'Echo',{'p'+str(i):('string','v') for i in range(16)},{'result':('string','done')});graph=self.get(g);graph['extension']={'keep':'metadata'};graph['functions']['Echo']['inputs']['p0']['hint']='keep this';graph['functions']['Echo']['extension']={'keep':True}
  for i in range(120):graph['nodes'].append({'id':'call'+str(i),'type':'function.call','function':'Echo','position':[100,100],'values':{'p'+str(k):'v' for k in range(16)},'metadata':{'keep':i}})
  path=self.workspace/'data/assets/BoundedMigration.cyasset';path.write_text(json.dumps({'format':'cygame-graph','schemaVersion':1,'graph':graph}),'utf-8');g=self.call('asset_open',{'path':'BoundedMigration.cyasset'})['graphId'];p={'graphId':g,'target':'function','name':'Echo','inputs':{'p0':port('string','v')},'outputs':{'result':port('string','done')}};plan=self.plan(p);self.assertTrue(plan['valid'],plan);self.assertTrue(plan['truncated']);self.assertEqual(plan['summary']['valuesRemoved'],1800);self.assertLessEqual(len(plan['changes']),512);self.assertLess(len(json.dumps(plan).encode()),400000);self.apply(p,plan,acceptLoss=True);result=self.get(g);self.assertEqual(result['extension'],{'keep':'metadata'});self.assertEqual(result['functions']['Echo']['inputs']['p0']['hint'],'keep this');self.assertEqual(result['nodes'][-1]['metadata'],{'keep':119})

if __name__=='__main__':unittest.main(verbosity=2)
