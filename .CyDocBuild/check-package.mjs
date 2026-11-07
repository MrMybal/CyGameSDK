import {readFile} from 'node:fs/promises';
import {dirname,join} from 'node:path';
import {fileURLToPath} from 'node:url';
import {createHash} from 'node:crypto';
const work=dirname(fileURLToPath(import.meta.url));
const lock=JSON.parse(await readFile(join(work,'package-lock.json'),'utf8'));
const actual='sha512-'+createHash('sha512').update(await readFile(process.argv[2])).digest('base64');
if(actual!==lock.packages['node_modules/cydoc'].integrity)throw Error('The CyDoc archive does not match the reviewed npm lock; do not bypass the integrity check.');
