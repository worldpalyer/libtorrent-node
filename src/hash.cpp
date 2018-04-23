#include <cctype>
#include <algorithm>
#include <mutex>
#include <cstring>
#include "hash.hpp"

namespace libtorrent {
namespace n {

std::string sha1_to_hex(lt::sha1_hash &hash) { return lt::aux::to_hex(hash); }

lt::sha1_hash sha1_from_hex(std::string &btih, lt::error_code &ec) {
    lt::sha1_hash info_hash;
    if (btih.size() == 40 + 9) {
        lt::aux::from_hex({&btih[9], 40}, info_hash.data());
        return info_hash;
    } else if (btih.size() == 40) {
        lt::aux::from_hex(btih, info_hash.data());
        return info_hash;
    } else if (btih.size() == 32 + 9) {
        std::string const ih = lt::base32decode(btih.substr(9));
        if (ih.size() != 20) {
            ec = errors::invalid_info_hash;
            return info_hash;
        }
        info_hash.assign(ih);
        return info_hash;
    } else if (btih.size() == 32) {
        std::string const ih = lt::base32decode(btih);
        if (ih.size() != 20) {
            ec = errors::invalid_info_hash;
            return info_hash;
        }
        info_hash.assign(ih);
        return info_hash;
    } else {
        ec = errors::invalid_info_hash;
        return info_hash;
    }
}

}  // namespace n
}  // namespace libtorrent