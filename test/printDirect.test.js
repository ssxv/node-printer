const fs = require('fs');
const path = require('path');
const assert = require('assert');

const root = path.join(__dirname, '..');
const indexPath = path.join(root, 'index.js');
const backupPath = indexPath + '.bak';

// Backup original
fs.copyFileSync(indexPath, backupPath);

// Create mock binding that returns a Promise for printDirect
const mock = `module.exports = { getPrinters: function(){ return []; }, printDirect: function(params){ return Promise.resolve(123); } }`;
fs.writeFileSync(indexPath, mock);

try{
  const printer = require('../printer');

  // Promise-based usage
  const p = printer.printDirect({ data: 'hello' });
  assert(p && typeof p.then === 'function', 'printDirect should return a Promise');
  p.then((res) => {
    assert.strictEqual(res, 123);
    console.log('printDirect Promise OK');

    // Callback-based usage
    printer.printDirect({ data: 'hi' }, function(err, res){
      if(err) throw err;
      assert.strictEqual(res, 123);
      console.log('printDirect callback OK');

      // done
      fs.copyFileSync(backupPath, indexPath);
      fs.unlinkSync(backupPath);
    });

  }).catch((e)=>{
    fs.copyFileSync(backupPath, indexPath);
    fs.unlinkSync(backupPath);
    throw e;
  });

}catch(e){
  // restore on error
  fs.copyFileSync(backupPath, indexPath);
  fs.unlinkSync(backupPath);
  throw e;
}
