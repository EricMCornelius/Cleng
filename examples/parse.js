var fs = require('fs');
var repl = require('repl');

var resources = ['macros', 'functions', 'structs', 'typedefs'];

var inst = repl.start({});
resources.forEach(function(resource) {
  inst.context[resource] = JSON.parse(fs.readFileSync(resource + '.json'));
});
