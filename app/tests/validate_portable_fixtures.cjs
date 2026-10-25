/* Optional development QA using the official Khronos validator, pinned in Docs/CommonAssetFormat.md. */
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const root = path.resolve(__dirname, '..', '..');
const validator = require(path.join(root, 'runtime', 'portable-validator', 'node_modules', 'gltf-validator'));
(async () => {
  const results = [];
  for (const name of ['StaticTriangle', 'AnimatedTriangle']) {
    const bytes = fs.readFileSync(path.join(root, 'app', 'examples', 'portable', name + '.glb'));
    const report = await validator.validateBytes(new Uint8Array(bytes), {
      uri: name + '.glb', maxIssues: 100,
      externalResourceFunction: () => Promise.reject(new Error('External resources are not allowed'))
    });
    fs.writeFileSync(path.join(root, 'data', 'reports', 'portable-khronos-' + name + '.json'), JSON.stringify(report, null, 2));
    results.push({name, validatorVersion: validator.version(), sha256: crypto.createHash('sha256').update(bytes).digest('hex'), errors: report.issues.numErrors, warnings: report.issues.numWarnings});
  }
  fs.writeFileSync(path.join(root, 'data', 'reports', 'portable-khronos-results.json'), JSON.stringify(results, null, 2));
  console.log(JSON.stringify(results, null, 2));
  if (results.some(r => r.errors !== 0)) process.exitCode = 1;
})().catch(error => { console.error(error); process.exitCode = 1; });
