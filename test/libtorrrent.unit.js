var lt = require("../lib/libtorrent.js");
var as = require("assert");
var fs = require("fs");
const { exec } = require('child_process');

describe('libtorrent', function () {
    it("libtorrent", function (done) {
        exec('rm -rf /tmp/abc', console.log);
        exec('mkdir /tmp/abc', console.log);
        lt.bootstrap({
            name: "libtorrent-node",
        });
        var task = lt.add_torrent({
            ti: "/Users/vty/Downloads/b.torrent",
            // magnet_uri: "magnet:?xt=urn:btih:901141ac0780a99079726eb73999d2ea20f5d5a0",
            save_path: "/tmp/abc",
        });
        task.status.pieces = null;
        console.log(task);
        var dowait = function (w, rate) {
            var ts = lt.list_torrent();
            for (var i = 0; i < ts.length; i++) {
                var t = ts[i];
                t.status.pieces = null;
                console.log(t);
                if (t.status.progress > rate) {
                    if (w === 0) {
                        return dofind(t);
                    } else {
                        return done();
                    }
                }
            }
            setTimeout(dowait, 1000, w, rate);
        };
        var dofind = function (t) {
            var nt = lt.find_torrent({
                hash: t.hash,
            });
            as.equal(nt.hash, t.hash);
            as.equal(lt.pause_torrent({ hash: t.hash }), true);
            as.equal(lt.resume_torrent({ hash: t.hash }), true);
            dowait(1, 0.000003);
        };
        dowait(0, 0.000001);
    });
});