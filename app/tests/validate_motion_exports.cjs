/* Optional local validation of actual animation-tool exports. */
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto');
const root=path.resolve(__dirname,'..','..');
const validator=require(path.join(root,'runtime','portable-validator','node_modules','gltf-validator'));
(async()=>{const results=[];for(const name of ['motion-ik','motion-root-restored','motion-trimmed']){
const bytes=fs.readFileSync(path.join(root,'data','reports',name+'.glb'));
const report=await validator.validateBytes(new Uint8Array(bytes),{uri:name+'.glb',maxIssues:1000,externalResourceFunction:()=>Promise.reject(new Error('External resources forbidden'))});
fs.writeFileSync(path.join(root,'data','reports',name+'-khronos.json'),JSON.stringify(report,null,2));
results.push({name,validatorVersion:validator.version(),sha256:crypto.createHash('sha256').update(bytes).digest('hex'),errors:report.issues.numErrors,warnings:report.issues.numWarnings});}
fs.writeFileSync(path.join(root,'data','reports','motion-khronos.json'),JSON.stringify(results,null,2));console.log(JSON.stringify(results,null,2));if(results.some(r=>r.errors||r.warnings))process.exitCode=1;
})().catch(e=>{console.error(e);process.exitCode=1});
