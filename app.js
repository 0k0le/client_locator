// Init express and fs
var express = require('express');
var app = express();
var fs = require('fs');
var bodyParser = require('body-parser');
var net = require('net');
var http = require('http');
var reload = require('reload');
var connect = require('connect');
var storage = require('node-sessionstorage');

app.set("view engine", "ejs");

const server = http.createServer(app);

var login = false;

app.use(bodyParser.urlencoded({extended : true}));
reload(app);

server.listen(3225, function () {
    console.log("Listening on port 3225");
    setInterval(function () {
        console.log("Server Status: stable");
    }, 60000);
});

app.post("/login", function (request, response) {
    console.log(request.body);
    if(request.body.username === 'okole' && request.body.password === 'a13731570863716117') {
        login = true;
        response.render('main');
    }

    response.render('login');
});

app.post("/refresh", function (request, response) {
    if(login === false)
        response.render('login');

    var client = new net.Socket();
    client.connect(2112, '127.0.0.1', function () {
        console.log('Connected');
        client.write("GET");
    });

    client.on('close', function () {
        console.log('Connection closed');
        response.redirect("main");
    });
});

app.get('/', function (req, res) {
    console.log("User connected from: " + req.ip);
    if(login === true) res.render('main');
    else res.render('login');
});

app.get('/login', function (req, res) {
    console.log("User connected from: " + req.ip);
    if(login === true) res.render('main');

    res.render('login');
});

app.get('/logout', function (req, res) {
    console.log("User requsting logout: " + req.ip);
    
    login = false;

    res.render('login');
});

app.post("/logout", function (request, response) { 
    login = false;
    response.render('login');
});