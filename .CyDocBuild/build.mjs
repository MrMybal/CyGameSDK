import {readFile,writeFile,cp,mkdir,rm} from 'node:fs/promises';
import {dirname,resolve,join,sep} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
import {execFileSync} from 'node:child_process';
import {importApi} from 'cydoc/api-import';
const work=dirname(fileURLToPath(import.meta.url)),root=resolve(work,'..');
const manifest=JSON.parse(await readFile(join(root,'app/CyTool.json'),'utf8'));
const config=JSON.parse(await readFile(join(work,'cydoc.config.json'),'utf8'));
if(manifest.version!==config.version)throw Error('Manual/catalogue versions differ');
await mkdir(join(work,'api'),{recursive:true});await mkdir(join(work,'internal'),{recursive:true});
const symbols={en:[],fr:[]};
for(const op of manifest.operations){
  const input=JSON.stringify(op.inputSchema,null,2),output=JSON.stringify(op.outputSchema,null,2);
  for(const locale of ['en','fr']){
    const fr=locale==='fr';const required=op.inputSchema.required||[];
    const behavior=fr?`Opération du SDK natif : \`${op.id}\`. Les paramètres sont validés selon le catalogue de cette version. L’éditeur doit être démarré.`:`Native SDK operation: \`${op.id}\`. ${op.description} Parameters are validated against this version's catalogue.`;
    const facts=fr?`Exécution asynchrone déclarée : ${op.canRunAsync?'oui':'non'}. Annulation déclarée : ${op.canCancel?'oui':'non'}. Suivi de progression : ${op.supportsProgress?'oui':'non'}. Indicateur natif d’annulation du document : ${op['x-editor-undoable']?'oui':'non'}. Ces déclarations ne prouvent pas la présence d’un codec ou d’un compilateur de jeu.`:`Declared async: ${!!op.canRunAsync}. Declared cancellation: ${!!op.canCancel}. Progress: ${!!op.supportsProgress}. Native Undo flag: ${!!op['x-editor-undoable']}. Flags do not prove that a game codec or compiler exists.`;
    symbols[locale].push({key:'operation',name:op.id,language:'python',kind:'operation',signature:`editor_call("${op.id}", parameters)`,description:behavior+'\n\n'+facts+'\n\n'+(fr?'Consultez le [cycle des jobs](doc:jobs) avant de terminer une tâche.':'Read the [job lifecycle](doc:jobs) before finishing a task.')+'\n\n## '+(fr?'Schéma des entrées':'Input schema')+' {#input-schema}\n\n```json\n'+input+'\n```\n\n## '+(fr?'Schéma du résultat':'Output schema')+' {#output-schema}\n\n```json\n'+output+'\n```',parameters:Object.entries(op.inputSchema.properties||{}).map(([name,s])=>({name,type:s.type?JSON.stringify(s.type):'JSON',default:s.default===undefined?'':JSON.stringify(s.default),description:fr?(required.includes(name)?'Obligatoire. Voir les contraintes du schéma.':'Facultatif. Voir les contraintes du schéma.'):(required.includes(name)?'Required. See schema constraints.':'Optional. See schema constraints.')})),returnType:'JSON',returns:fr?'Résultat de l’opération ; pour un job, attendre son état terminal et lire ses sorties.':'Operation result; for a job, wait for its terminal state and inspect outputs.',source:{file:'app/CyTool.json',line:1}});
  }
}
for(const lang of ['en','fr'])await writeFile(join(work,`api/operations-${lang}.json`),JSON.stringify({symbols:symbols[lang]},null,2)+'\n');
// The public neutral importer supplies signatures, parameter tables and returns.
// App-owned Markdown input adds parent/revision metadata absent from this importer.
const operationReviews=JSON.parse(await readFile(join(work,'operation-reviews.json'),'utf8'));
for(let i=0;i<manifest.operations.length;++i){
 const fingerprint='sha256:'+createHash('sha256').update(JSON.stringify({en:symbols.en[i],fr:symbols.fr[i]})).digest('hex');
 if(operationReviews[manifest.operations[i].id]!==fingerprint)throw Error(`Review the generated operation translations: ${manifest.operations[i].id}`);
}
if(Object.keys(operationReviews).length!==manifest.operations.length)throw Error('Obsolete operation translation reviews');
const imported=await importApi(['en','fr'].map(locale=>({format:'cydoc',path:`api/operations-${locale}.json`,locale,prefix:'op'})),{base:work,locales:config.locales});
for(const lang of ['en','fr']){
 const generated=join(work,'manual',lang,'api-generated');
 // Verify the resolved target remains in the owned generated-input area.
 if(!generated.startsWith(resolve(work,'manual')+sep)||!generated.endsWith(sep+'api-generated'))throw Error('Unsafe generated-input path');
 await rm(generated,{recursive:true,force:true});await mkdir(generated,{recursive:true});
 for(const p of imported.filter(p=>p.locale===lang)){
  const revision=operationReviews[p.title];
  const meta=`---\nid: ${p.id}\ntitle: ${p.title}\ndescription: ${lang==='fr'?'Référence de l’opération':'Operation reference'} ${p.title}\nsection: SDK\nparent: operations\nkind: api\nsymbol: ${p.symbol}\norder: 250\n${lang==='en'?'revision':'sourceRevision'}: ${revision}\n---\n\n`;
  await writeFile(join(generated,p.id+'.md'),meta+p.markdown+'\n');
 }
}
const packageRoot=join(work,'node_modules/cydoc');
const pkg=JSON.parse(await readFile(join(packageRoot,'package.json'),'utf8'));
execFileSync(process.execPath,[join(packageRoot,pkg.bin.cydoc),process.argv.includes('--check')?'check':'build',join(work,'cydoc.config.json')],{cwd:root,stdio:'inherit'});
const result={output:join(root,'Documentation'),data:JSON.parse(await readFile(join(root,'Documentation/DocData/manifest.json'),'utf8'))};
const report=JSON.parse(await readFile(join(result.output,'DocData/translation-report.json'),'utf8'));
if(report.missing.length||report.outdated.length)throw Error('Incomplete/outdated translations');
const reviewPath=join(work,'translation-review.json');
const reviews=JSON.parse(await readFile(reviewPath,'utf8'));
for(const item of report.untracked){
  const en=result.data.pages.find(p=>p.id===item.id&&p.locale==='en');
  const fr=result.data.pages.find(p=>p.id===item.id&&p.locale===item.locale);
  const fingerprint='sha256:'+createHash('sha256').update(JSON.stringify({en,fr})).digest('hex');
  if(reviews[item.id]!==fingerprint)throw Error(`Review the EN/FR generated reference pair ${item.id}, then update translation-review.json explicitly`);
}
if(Object.keys(reviews).length!==report.untracked.length)throw Error('Translation review ledger contains obsolete IDs');
// Packaging-only cache keys: preserve the official reader and bootstrap; do not
// alter data/manifest contents or implement a second reader.
const entry=join(result.output,'Documentation.html');let entryHtml=await readFile(entry,'utf8');
for(const file of ['viewer.css','viewer.js','data.js']){
 const hash=createHash('sha256').update(await readFile(join(result.output,'DocData',file))).digest('hex').slice(0,12);
 entryHtml=entryHtml.replaceAll(`DocData/${file}\"`,`DocData/${file}?v=${hash}\"`);
}
await writeFile(entry,entryHtml);
const modules=config.modules;
for(const name of ['mesh','video','gif'])if(modules.includes(name))throw Error('Unused reader selected');
await cp(join(root,'app/LICENCES/CyDoc-MIT.txt'),join(result.output,'DocData/CyDoc-MIT.txt'));
await cp(join(root,'app/LICENCES/README.md'),join(result.output,'DocData/DEPENDENCY-NOTICES.md'));
// Mirror the complete official export for the static project site, preserving its reader.
const web=join(root,'docs/manual');await mkdir(web,{recursive:true});await cp(result.output,web,{recursive:true});
await import('./verify.mjs');
const hashes=Object.fromEntries(result.data.pages.filter(p=>p.locale==='en').map(p=>[p.id,p.revision]));
await writeFile(join(work,'internal/compiled-revisions.json'),JSON.stringify(hashes,null,2)+'\n');
console.log(`CyGameSDK: ${manifest.operations.length} operation references per locale; readers: ${modules.join(', ')}; complete export and website copy.`);
