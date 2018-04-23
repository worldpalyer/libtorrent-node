var lt = require("../lib/libtorrent.js");
lt.bootstrap({
    name: "libtorrent-node",
});
lt.add_torrent({
    ti: "/Users/vty/Downloads/a.torrent",
    // magnet_uri: "magnet:?xt=urn:btih:901141ac0780a99079726eb73999d2ea20f5d5a0",
    save_path: "/tmp/abc",
});

setTimeout(function () { }, 1000000);