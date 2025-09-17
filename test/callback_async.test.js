const fs = require('fs');
const path = require('path');
const assert = require('assert');

const root = path.join(__dirname, '..');
const indexPath = path.join(root, 'index.js');
const backupPath = indexPath + '.bak';

// Backup original
fs.copyFileSync(indexPath, backupPath);

// Mock binding: synchronous printFile
const mock = `module.exports = { getPrinters: function(){ return []; }, printFile: function(filename){ return 123; } }`;
fs.writeFileSync(indexPath, mock);

try{
  const printer = require('../printer');

  let called = false;

  // call printFile with callback; native returns synchronously
  printer.printFile({ filename: 'README.md' }, function(err, res){
    try{
      if(err) throw err;
      assert.strictEqual(res, 123);
      called = true;
      console.log('callback async OK');

      // restore
      fs.copyFileSync(backupPath, indexPath);
      fs.unlinkSync(backupPath);
    }catch(e){
      fs.copyFileSync(backupPath, indexPath);
      fs.unlinkSync(backupPath);
      throw e;
    }
  });

  // The callback must not be called synchronously
  assert.strictEqual(called, false, 'callback should be invoked asynchronously');

}catch(e){
  // restore on error
  fs.copyFileSync(backupPath, indexPath);
  fs.unlinkSync(backupPath);
  throw e;
}

