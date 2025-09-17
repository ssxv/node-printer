const fs = require('fs');
const path = require('path');
const assert = require('assert');

const root = path.join(__dirname, '..');
const indexPath = path.join(root, 'index.js');
const backupPath = indexPath + '.bak';

// Backup original
fs.copyFileSync(indexPath, backupPath);

// Mock binding: returns a Promise for printFile
const mock = `module.exports = { getPrinters: function(){ return []; }, printFile: function(filename){ return Promise.resolve(789); } }`;
fs.writeFileSync(indexPath, mock);

try{
  const printer = require('../printer');

  let callbackCalled = 0;
  let promiseResolved = 0;

  const p = printer.printFile({ filename: 'README.md' }, function(err, res){
    if(err) throw err;
    callbackCalled++;
    assert.strictEqual(res, 789);
  });

  p.then(function(res){
    promiseResolved++;
    assert.strictEqual(res, 789);

    // allow one tick for callback to run
    setImmediate(function(){
      try{
        assert.strictEqual(callbackCalled, 1, 'callback should be called exactly once');
        assert.strictEqual(promiseResolved, 1, 'promise should resolve exactly once');
        console.log('no double-reporting OK');

        // restore
        fs.copyFileSync(backupPath, indexPath);
        fs.unlinkSync(backupPath);
      }catch(e){
        fs.copyFileSync(backupPath, indexPath);
        fs.unlinkSync(backupPath);
        throw e;
      }
    });
  }).catch(function(e){
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
