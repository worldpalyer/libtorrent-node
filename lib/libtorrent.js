// try {
//    console.log(__dirname + '/../build/Release/emulexnode.node');
var torrentnode = require(__dirname + '/../build/Release/torrentnode.node');
// } catch (error) {
//     emulex = require(__dirname + '/../build/default/emulex.node');
// }
exports.bootstrap = torrentnode.bootstrap;
exports.shutdown = torrentnode.shutdown;
exports.read_torrent = torrentnode.read_torrent;
exports.add_torrent = torrentnode.add_torrent;
exports.list_torrent = torrentnode.list_torrent;
exports.find_torrent = torrentnode.find_torrent;
exports.pause_torrent = torrentnode.pause_torrent;
exports.resume_torrent = torrentnode.resume_torrent;
exports.remove_torrent = torrentnode.remove_torrent;
exports.save_resume = torrentnode.save_resume;
