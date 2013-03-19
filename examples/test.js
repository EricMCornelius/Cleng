#!/usr/bin/env node

var fs = require('fs');
var repl = require('repl');

var resources = ['output'];

function types(nodes) {
  var types = {};
  function iter(arr) {
    arr.forEach(function(n) {
      if (!n.node_type)
        return;

      types[n.node_type] = (types[n.node_type] || []).concat(n);
    
      if (n.context)
        iter(n.context);
    });
  };
  iter(nodes);
  return types;
}

function print_field(field) {
  return function(node) {
    console.log(node[field]);
  }
}

var inst = repl.start({});
resources.forEach(function(resource) {
  inst.context[resource] = JSON.parse(fs.readFileSync(resource + '.json'));
});

inst.context['types'] = types;
inst.context['print_field'] = print_field;