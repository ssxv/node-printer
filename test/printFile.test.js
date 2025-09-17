const fs = require('fs');
const path = require('path');
const assert = require('assert');

const root = path.join(__dirname, '..');
const indexPath = path.join(root, 'index.js');
const backupPath = indexPath + '.bak';

// Backup original
fs.copyFileSync(indexPath, backupPath);

// Create mock binding that returns a Promise for printFile
const mock = `module.exports = { getPrinters: function(){ return []; }, printFile: function(filename){ return Promise.resolve(456); } }`;
fs.writeFileSync(indexPath, mock);

try{
  const printer = require('../printer');

  // Promise-based usage
  const p = printer.printFile({ filename: 'README.md' });
  assert(p && typeof p.then === 'function', 'printFile should return a Promise');
  p.then((res) => {
    assert.strictEqual(res, 456);
    console.log('printFile Promise OK');

    // Callback-based usage
    printer.printFile({ filename: 'README.md' }, function(err, res){
      if(err) throw err;
      assert.strictEqual(res, 456);
      console.log('printFile callback OK');

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
