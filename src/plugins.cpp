//
//  plugins.cpp
//  emulex
//
//  Created by Centny on 2/5/17.
//
//

#include "plugins.hpp"
#include <node.h>
#include <node_buffer.h>
#include <stdio.h>
#include <v8.h>
#include <uv.h>
#include <string>
#include "common.h"
#include <cstdlib>
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include "libtorrent/torrent_info.hpp"
#include "libtorrent/magnet_uri.hpp"
#include <iostream>
#include "hash.hpp"

using namespace node;
using namespace v8;

namespace libtorrent
{
namespace n
{
struct xloader
{
    lt::session *session = 0;
    Local<Object> settings;
    bool running = false;
    Persistent<Function> shutdown_callback;
};

static xloader XL;

int cache_size = 1024;

void bootstrap(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    if (args.Length() < 1 || !args[0]->IsObject())
    {
        StringException(isolate, "bootstrap receive bad argument, the setting is not object");
        return;
    }
    XL.settings = args[0]->ToObject();
    Local<Value> enable_upnp = XL.settings->Get(String::NewFromUtf8(isolate, "enable_upnp"));
    Local<Value> enable_natpmp = XL.settings->Get(String::NewFromUtf8(isolate, "enable_natpmp"));
    Local<Value> enable_lsd = XL.settings->Get(String::NewFromUtf8(isolate, "enable_lsd"));
    Local<Value> enable_dht = XL.settings->Get(String::NewFromUtf8(isolate, "enable_dht"));

    Local<Value> name_v = XL.settings->Get(String::NewFromUtf8(isolate, "name"));
    if (!name_v->IsString())
    {
        StringException(isolate, "libtorrent bootstrap fail with args.name is not string");
        return;
    }
    String::Utf8Value name_s(name_v->ToString());
    std::string name = std::string(*name_s, name_s.length());
    //
    lt::error_code ec;
    lt::session_params params;
    params.dht_settings.privacy_lookups = true;
    auto &settings = params.settings;
    settings.set_int(settings_pack::cache_size, cache_size);
    settings.set_int(settings_pack::choking_algorithm, settings_pack::rate_based_choker);
    settings.set_str(settings_pack::user_agent, name + "/" LIBTORRENT_VERSION);
    settings.set_int(settings_pack::alert_mask,
                     alert::error_notification | alert::peer_notification | alert::port_mapping_notification |
                         alert::storage_notification | alert::tracker_notification | alert::debug_notification |
                         alert::status_notification | alert::ip_block_notification | alert::performance_warning |
                         alert::dht_notification | alert::incoming_request_notification |
                         alert::dht_operation_notification | alert::port_mapping_log_notification |
                         alert::file_progress_notification);
    params.settings.set_bool(settings_pack::enable_upnp, enable_upnp->IsBoolean() && enable_upnp->BooleanValue());
    params.settings.set_bool(settings_pack::enable_natpmp, enable_natpmp->IsBoolean() &&
                                                               enable_natpmp->BooleanValue());
    params.settings.set_bool(settings_pack::enable_lsd, enable_lsd->IsBoolean() &&
                                                            enable_lsd->BooleanValue());
    params.settings.set_bool(settings_pack::enable_dht, enable_dht->IsBoolean() &&
                                                            enable_dht->BooleanValue());
    XL.session = new lt::session(std::move(params));
    // lt::ip_filter loaded_ip_filter;
    // XL.session->set_ip_filter(loaded_ip_filter);
}

void shutdown(const FunctionCallbackInfo<Value> &args)
{
    if (XL.session)
    {
        XL.session->pause();
        XL.session = 0;
    }
}

Local<Object> parse_status_value(Isolate *isolate, lt::torrent_status status)
{
    Local<Object> obj = Object::New(isolate);
    obj->Set(String::NewFromUtf8(isolate, "progress"), Number::New(isolate, status.progress));
    obj->Set(String::NewFromUtf8(isolate, "total_done"), Number::New(isolate, (uint32_t)status.total_done));
    obj->Set(String::NewFromUtf8(isolate, "total_done_h"), Number::New(isolate, (uint32_t)(status.total_done >> 32)));
    obj->Set(String::NewFromUtf8(isolate, "total_wanted_done"),
             Number::New(isolate, (uint32_t)status.total_wanted_done));
    obj->Set(String::NewFromUtf8(isolate, "total_wanted_done_h"),
             Number::New(isolate, (uint32_t)(status.total_wanted_done >> 32)));
    obj->Set(String::NewFromUtf8(isolate, "total_wanted"), Number::New(isolate, (uint32_t)status.total_wanted));
    obj->Set(String::NewFromUtf8(isolate, "total_wanted_h"),
             Number::New(isolate, (uint32_t)(status.total_wanted >> 32)));
    obj->Set(String::NewFromUtf8(isolate, "download_rate"), Number::New(isolate, (uint32_t)status.download_rate));
    obj->Set(String::NewFromUtf8(isolate, "upload_rate"), Number::New(isolate, (uint32_t)status.upload_rate));
    lt::typed_bitfield<lt::piece_index_t> pieces = status.pieces;
    int psize = pieces.size();
    if (psize)
    {
        char *pdata = pieces.data();
        obj->Set(String::NewFromUtf8(isolate, "pieces"),
                 Uint8Array::New(ArrayBuffer::New(isolate, (void *)pdata, psize), 0, psize));
    }
    else
    {
        obj->Set(String::NewFromUtf8(isolate, "pieces"), Null(isolate));
    }
    return obj;
}

Local<Object> parse_torrent_handle(Isolate *isolate, lt::torrent_handle &th)
{
    Local<Object> result = Object::New(isolate);
    result->Set(String::NewFromUtf8(isolate, "is_valid"), Boolean::New(isolate, th.is_valid()));
    if (th.is_valid())
    {
        result->Set(String::NewFromUtf8(isolate, "status"), parse_status_value(isolate, th.status()));
    }
    auto info_hash = th.info_hash();
    std::string hash = sha1_to_hex(info_hash);
    result->Set(String::NewFromUtf8(isolate, "hash"), String::NewFromUtf8(isolate, hash.c_str()));
    return result;
}

void add_torrent(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    if (!XL.session)
    {
        StringException(isolate, "libtorrent is not running");
        return;
    }
    if (!(args.Length() > 0 && args[0]->IsObject()))
    {
        StringException(isolate, "libtorrent add torrent receive wrong arguments");
        return;
    }
    Local<Object> vargs = args[0]->ToObject();
    //
    std::string save_path;
    Local<Value> save_path_v = vargs->Get(String::NewFromUtf8(isolate, "save_path"));
    if (!save_path_v->IsString())
    {
        StringException(isolate, "libtorrent add torrent fail with args.save_path is not string");
        return;
    }
    String::Utf8Value save_path_s(save_path_v->ToString());
    save_path = std::string(*save_path_s, save_path_s.length());
    //
    lt::error_code ec;
    lt::add_torrent_params params;
    Local<Value> ti_v = vargs->Get(String::NewFromUtf8(isolate, "ti"));
    Local<Value> magnet_uri_v = vargs->Get(String::NewFromUtf8(isolate, "magnet_uri"));
    if (ti_v->IsString())
    {
        std::string ti;
        String::Utf8Value ti_s(ti_v->ToString());
        ti = std::string(*ti_s, ti_s.length());
        params.ti = std::make_shared<lt::torrent_info>(ti, ec);
        if (ec)
        {
            std::string msg = "read torrent info from " + ti + " fail with " + ec.message();
            StringException(isolate, msg.c_str());
            return;
        }
    }
    else if (magnet_uri_v->IsString())
    {
        std::string magnet_uri;
        String::Utf8Value magnet_uri_s(magnet_uri_v->ToString());
        magnet_uri = std::string(*magnet_uri_s, magnet_uri_s.length());
        params = lt::parse_magnet_uri(lt::string_view(magnet_uri), ec);
        if (ec)
        {
            std::string msg = "parse magnet uri " + magnet_uri + " fail with " + ec.message();
            StringException(isolate, msg.c_str());
            return;
        }
    }
    else
    {
        StringException(isolate, "libtorrent add torrent fail with args.ti is not string");
        return;
    }

    //
    params.save_path = save_path;
    lt::torrent_handle th = XL.session->add_torrent(params, ec);
    if (ec)
    {
        std::string msg = "add torrent fail with " + ec.message();
        StringException(isolate, msg.c_str());
        return;
    }
    args.GetReturnValue().Set(parse_torrent_handle(isolate, th));
}

void find_torrent(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    if (!XL.session)
    {
        StringException(isolate, "libtorrent is not running");
        return;
    }
    if (!(args.Length() > 0 && args[0]->IsObject()))
    {
        StringException(isolate, "libtorrent find torrent receive wrong arguments");
        return;
    }
    Local<Object> vargs = args[0]->ToObject();
    //
    Local<Value> hash_v = vargs->Get(String::NewFromUtf8(isolate, "hash"));
    if (!hash_v->IsString())
    {
        StringException(isolate, "libtorrent find torrent fail with args.hash is not string");
        return;
    }
    String::Utf8Value hash_s(hash_v->ToString());
    std::string hash = std::string(*hash_s, hash_s.length());
    lt::error_code ec;
    lt::sha1_hash sha1 = sha1_from_hex(hash, ec);
    if (ec)
    {
        StringException(isolate, ("libtorrent find torrent fail with parse args.hash " + ec.message()).c_str());
        return;
    }
    //
    lt::torrent_handle th = XL.session->find_torrent(sha1);
    args.GetReturnValue().Set(parse_torrent_handle(isolate, th));
}

void pause_torrent(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    if (!XL.session)
    {
        StringException(isolate, "libtorrent is not running");
        return;
    }
    if (!(args.Length() > 0 && args[0]->IsObject()))
    {
        StringException(isolate, "libtorrent pause torrent receive wrong arguments");
        return;
    }
    Local<Object> vargs = args[0]->ToObject();
    //
    Local<Value> hash_v = vargs->Get(String::NewFromUtf8(isolate, "hash"));
    if (!hash_v->IsString())
    {
        StringException(isolate, "libtorrent pause torrent fail with args.hash is not string");
        return;
    }
    String::Utf8Value hash_s(hash_v->ToString());
    std::string hash = std::string(*hash_s, hash_s.length());
    lt::error_code ec;
    lt::sha1_hash sha1 = sha1_from_hex(hash, ec);
    if (ec)
    {
        StringException(isolate, ("libtorrent pause torrent fail with parse args.hash " + ec.message()).c_str());
        return;
    }
    //
    lt::torrent_handle th = XL.session->find_torrent(sha1);
    if (th.is_valid())
    {
        th.pause();
    }
    args.GetReturnValue().Set(Boolean::New(isolate, th.is_valid()));
}

void resume_torrent(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    if (!XL.session)
    {
        StringException(isolate, "libtorrent is not running");
        return;
    }
    if (!(args.Length() > 0 && args[0]->IsObject()))
    {
        StringException(isolate, "libtorrent resume torrent receive wrong arguments");
        return;
    }
    Local<Object> vargs = args[0]->ToObject();
    //
    Local<Value> hash_v = vargs->Get(String::NewFromUtf8(isolate, "hash"));
    if (!hash_v->IsString())
    {
        StringException(isolate, "libtorrent resume torrent fail with args.hash is not string");
        return;
    }
    String::Utf8Value hash_s(hash_v->ToString());
    std::string hash = std::string(*hash_s, hash_s.length());
    lt::error_code ec;
    lt::sha1_hash sha1 = sha1_from_hex(hash, ec);
    if (ec)
    {
        StringException(isolate, ("libtorrent resume torrent fail with parse args.hash " + ec.message()).c_str());
        return;
    }
    //
    lt::torrent_handle th = XL.session->find_torrent(sha1);
    if (th.is_valid())
    {
        th.resume();
    }
    args.GetReturnValue().Set(Boolean::New(isolate, th.is_valid()));
}

void remove_torrent(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    if (!XL.session)
    {
        StringException(isolate, "libtorrent is not running");
        return;
    }
    if (!(args.Length() > 0 && args[0]->IsObject()))
    {
        StringException(isolate, "libtorrent remove torrent receive wrong arguments");
        return;
    }
    Local<Object> vargs = args[0]->ToObject();
    //
    Local<Value> hash_v = vargs->Get(String::NewFromUtf8(isolate, "hash"));
    if (!hash_v->IsString())
    {
        StringException(isolate, "libtorrent resume torrent fail with args.hash is not string");
        return;
    }
    String::Utf8Value hash_s(hash_v->ToString());
    std::string hash = std::string(*hash_s, hash_s.length());
    lt::error_code ec;
    lt::sha1_hash sha1 = sha1_from_hex(hash, ec);
    if (ec)
    {
        StringException(isolate, ("libtorrent remove torrent fail with parse args.hash " + ec.message()).c_str());
        return;
    }
    //
    lt::torrent_handle th = XL.session->find_torrent(sha1);
    if (th.is_valid())
    {
        XL.session->remove_torrent(th, lt::session::delete_files);
    }
    args.GetReturnValue().Set(Boolean::New(isolate, th.is_valid()));
}

void list_torrent(const FunctionCallbackInfo<Value> &args)
{
    Isolate *isolate = args.GetIsolate();
    if (!XL.session)
    {
        StringException(isolate, "libtorrent is not running");
        return;
    }
    //
    std::vector<lt::torrent_handle> handles = XL.session->get_torrents();
    Local<Array> torrents = Array::New(isolate, handles.size());
    for (size_t i = 0; i < handles.size(); i++)
    {
        lt::torrent_handle &th = handles[i];
        torrents->Set(i, parse_torrent_handle(isolate, th));
    }
    args.GetReturnValue().Set(torrents);
}

void init(Local<Object> exports)
{
    NODE_SET_METHOD(exports, "bootstrap", bootstrap);
    NODE_SET_METHOD(exports, "shutdown", shutdown);
    NODE_SET_METHOD(exports, "add_torrent", add_torrent);
    NODE_SET_METHOD(exports, "list_torrent", list_torrent);
    NODE_SET_METHOD(exports, "find_torrent", find_torrent);
    NODE_SET_METHOD(exports, "pause_torrent", pause_torrent);
    NODE_SET_METHOD(exports, "resume_torrent", resume_torrent);
    NODE_SET_METHOD(exports, "remove_torrent", resume_torrent);
}

NODE_MODULE(libtorrent, init);
///
} // namespace n
} // namespace libtorrent
