#!/usr/bin/env node

var fs = require('fs');
var _ = require('lodash');

var files = process.argv.slice(2);
var contents = files.map(function(file) {
  return fs.readFileSync(file);
}).map(JSON.parse);

var fields = ['macros', 'functions', 'enums'];
var output = _.reduce(contents, function(agg, file) {
  _.reduce(fields, function(agg, field) {
    agg[field] = (agg[field] || []).concat(file[field]);
    return agg;
  }, agg);
  return agg;
}, {});

console.log(JSON.stringify(output, null, 2));
