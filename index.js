'use strict';

const desktop = require("./dist/desktop-native.node");

module.exports.activateWindow = desktop.activateWindow;
module.exports.listWindows = desktop.listWindows;
module.exports.listApplications = desktop.listApplications;