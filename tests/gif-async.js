var fs  = require('fs');
var sys = require('sys');
var Gif = require('gif').Gif;
var Buffer = require('buffer').Buffer;

var terminal = fs.readFileSync('./terminal.rgb');

var gif = new Gif(terminal, 720, 400, 'rgb');

gif.encode(function (data) {
    fs.writeFileSync('./terminal-async.gif', data.toString('binary'), 'binary');
});


