import assert from 'node:assert/strict';
import {readFile,readdir,stat} from 'node:fs/promises';
import {resolve,dirname,join} from 'node:path';
import {fileURLToPath} from 'node:url';
const root=resolve(dirname(fileURLToPath(import.meta.url)),'..');
const output=join(root,'Documentation'),data=JSON.parse(await readFile(join(output,'DocData/manifest.json'),'utf8'));
const ops=JSON.parse(await readFile(join(root,'app/CyTool.json'),'utf8')).operations;
const en=data.pages.filter(p=>p.locale==='en'),fr=data.pages.filter(p=>p.locale==='fr');
assert.deepEqual(en.map(p=>p.id).sort(),fr.map(p=>p.id).sort(),'EN/FR page IDs');
for(const p of en){const t=fr.find(t=>t.id===p.id);assert.deepEqual(p.headings.map(h=>h.id),t.headings.map(h=>h.id),`Translated anchors: ${p.id}`);}
assert.equal(en.filter(p=>p.id.startsWith('op-')).length,ops.length);
for(const op of ops)assert(en.some(p=>p.title===op.id&&p.id.startsWith('op-')),`Missing operation ${op.id}`);
const native=await readFile(join(root,'app/native/Documentation.cpp'),'utf8');
for(const [,page,anchor] of native.matchAll(/\{\"([a-z][a-z0-9-]+)\",\"([a-z][a-z0-9-]+)\"\}/g)){
 const p=en.find(p=>p.id===page);assert(p,`Help page ${page}`);assert(p.headings.some(h=>h.id===anchor),`Help anchor ${page}/${anchor}`);
}
const html=await readFile(join(output,'Documentation.html'),'utf8');
assert(!/<(?:script|link)[^>]+(?:src|href)=\"(?:https?:)?\/\//i.test(html),'Remote reader dependency');
for(const [,path]of html.matchAll(/(?:src|href)=\"(DocData\/[^\"]+)\"/g))assert((await stat(join(output,path.split('?')[0]))).isFile(),`Local export ${path}`);
const files=await readdir(join(output,'DocData'));
for(const module of ['image','gallery','graph'])assert(files.some(f=>f===`media-${module}.js`),`Selected reader ${module}`);
for(const module of ['mesh','video','gif'])assert(!files.some(f=>f===`media-${module}.js`),`Unused reader ${module}`);
assert(!files.some(f=>/three/i.test(f)),'No Three.js export');
assert(!(await readdir(join(root,'.CyDocBuild/node_modules'))).includes('three'),'No application Three.js dependency');
for(const module of ['image','gallery','graph'])assert(en.some(p=>p.html.includes(`data-cydoc-module=\"${module}\"`)),`Used module ${module}`);
for(const p of data.pages){assert(!/D:[\\/]|C:[\\/]Users[\\/]/.test(p.html),`Private absolute path ${p.id}`);}
console.log(`CyGameSDK documentation contracts: ${en.length} page IDs, EN/FR anchors, ${ops.length} operations, native help, local assets, three selected readers: PASS`);
