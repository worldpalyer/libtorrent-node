#include <string>
#include "libtorrent/assert.hpp"
#include "libtorrent/parse_url.hpp"
#include "libtorrent/utf8.hpp"
#include "libtorrent/string_util.hpp"  // for to_string
#include "libtorrent/aux_/array.hpp"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/magnet_uri.hpp"
#include "libtorrent/hex.hpp"
namespace libtorrent {
namespace n {
std::string sha1_to_hex(lt::sha1_hash &hash);
lt::sha1_hash sha1_from_hex(std::string &btih, lt::error_code &ec);
}  // namespace n
}  // namespace libtorrent