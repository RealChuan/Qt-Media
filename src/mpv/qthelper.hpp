#ifndef LIBMPV_QTHELPER_H_
#define LIBMPV_QTHELPER_H_

#include <mpv/client.h>

#include <cstring>

#include <QHash>
#include <QList>
#include <QMetaType>
#include <QSharedPointer>
#include <QString>
#include <QVariant>

namespace mpv {
namespace qt {

// Wrapper around mpv_handle. Does refcounting under the hood.
class Handle
{
    struct container
    {
        explicit container(mpv_handle *h)
            : mpv(h)
        {}
        ~container() { mpv_terminate_destroy(mpv); }
        mpv_handle *mpv;
    };
    QSharedPointer<container> sptr;

public:
    // Construct a new Handle from a raw mpv_handle with refcount 1. If the
    // last Handle goes out of scope, the mpv_handle will be destroyed with
    // mpv_terminate_destroy().
    // Never destroy the mpv_handle manually when using this wrapper. You
    // will create dangling pointers. Just let the wrapper take care of
    // destroying the mpv_handle.
    // Never create multiple wrappers from the same raw mpv_handle; copy the
    // wrapper instead (that's what it's for).
    static auto FromRawHandle(mpv_handle *handle) -> Handle
    {
        Handle h;
        h.sptr = QSharedPointer<container>(new container(handle));
        return h;
    }

    // Return the raw handle; for use with the libmpv C API.
    explicit operator mpv_handle *() const { return sptr ? (*sptr).mpv : 0; }
};

static inline auto node_to_variant(const mpv_node *node) -> QVariant
{
    switch (node->format) {
    case MPV_FORMAT_STRING: return QVariant(QString::fromUtf8(node->u.string));
    case MPV_FORMAT_FLAG: return QVariant(static_cast<bool>(node->u.flag));
    case MPV_FORMAT_INT64: return QVariant(static_cast<qlonglong>(node->u.int64));
    case MPV_FORMAT_DOUBLE: return QVariant(node->u.double_);
    case MPV_FORMAT_NODE_ARRAY: {
        mpv_node_list *list = node->u.list;
        QVariantList qlist;
        for (int n = 0; n < list->num; n++) {
            qlist.append(node_to_variant(&list->values[n]));
        }
        return QVariant(qlist);
    }
    case MPV_FORMAT_NODE_MAP: {
        mpv_node_list *list = node->u.list;
        QVariantMap qmap;
        for (int n = 0; n < list->num; n++) {
            qmap.insert(QString::fromUtf8(list->keys[n]), node_to_variant(&list->values[n]));
        }
        return QVariant(qmap);
    }
    default: // MPV_FORMAT_NONE, unknown values (e.g. future extensions)
        return QVariant();
    }
}

struct node_builder
{
    explicit node_builder(const QVariant &v) { set(&node_, v); }
    ~node_builder() { free_node(&node_); }
    auto node() -> mpv_node * { return &node_; }

private:
    Q_DISABLE_COPY(node_builder)
    mpv_node node_;
    auto create_list(mpv_node *dst, bool is_map, int num) -> mpv_node_list *
    {
        dst->format = is_map ? MPV_FORMAT_NODE_MAP : MPV_FORMAT_NODE_ARRAY;
        auto *list = new mpv_node_list();
        dst->u.list = list;
        if (list == nullptr) {
            goto err;
        }
        list->values = new mpv_node[num]();
        if (list->values == nullptr) {
            goto err;
        }
        if (is_map) {
            list->keys = new char *[num]();
            if (list->keys == nullptr) {
                goto err;
            }
        }
        return list;
    err:
        free_node(dst);
        return nullptr;
    }
    auto dup_qstring(const QString &s) -> char *
    {
        QByteArray b = s.toUtf8();
        char *r = new char[b.size() + 1];
        if (r != nullptr)
            std::memcpy(r, b.data(), b.size() + 1);
        return r;
    }
    auto test_type(const QVariant &v, QMetaType::Type t) -> bool
    {
        // The Qt docs say: "Although this function is declared as returning
        // "QVariant::Type(obsolete), the return value should be interpreted
        // as QMetaType::Type."
        // So a cast really seems to be needed to avoid warnings (urgh).
        return static_cast<int>(v.type()) == static_cast<int>(t);
    }
    void set(mpv_node *dst, const QVariant &src)
    {
        if (test_type(src, QMetaType::QString)) {
            dst->format = MPV_FORMAT_STRING;
            dst->u.string = dup_qstring(src.toString());
            if (dst->u.string == nullptr) {
                goto fail;
            }
        } else if (test_type(src, QMetaType::Bool)) {
            dst->format = MPV_FORMAT_FLAG;
            dst->u.flag = src.toBool() ? 1 : 0;
        } else if (test_type(src, QMetaType::Int) || test_type(src, QMetaType::LongLong)
                   || test_type(src, QMetaType::UInt) || test_type(src, QMetaType::ULongLong)) {
            dst->format = MPV_FORMAT_INT64;
            dst->u.int64 = src.toLongLong();
        } else if (test_type(src, QMetaType::Double)) {
            dst->format = MPV_FORMAT_DOUBLE;
            dst->u.double_ = src.toDouble();
        } else if (src.canConvert<QVariantList>()) {
            QVariantList qlist = src.toList();
            mpv_node_list *list = create_list(dst, false, qlist.size());
            if (list == nullptr) {
                goto fail;
            }
            list->num = qlist.size();
            for (int n = 0; n < qlist.size(); n++) {
                set(&list->values[n], qlist[n]);
            }
        } else if (src.canConvert<QVariantMap>()) {
            QVariantMap qmap = src.toMap();
            mpv_node_list *list = create_list(dst, true, qmap.size());
            if (list == nullptr) {
                goto fail;
            }
            list->num = qmap.size();
            for (int n = 0; n < qmap.size(); n++) {
                list->keys[n] = dup_qstring(qmap.keys()[n]);
                if (list->keys[n] == nullptr) {
                    free_node(dst);
                    goto fail;
                }
                set(&list->values[n], qmap.values()[n]);
            }
        } else {
            goto fail;
        }
        return;
    fail:
        dst->format = MPV_FORMAT_NONE;
    }
    void free_node(mpv_node *dst)
    {
        switch (dst->format) {
        case MPV_FORMAT_STRING: delete[] dst->u.string; break;
        case MPV_FORMAT_NODE_ARRAY:
        case MPV_FORMAT_NODE_MAP: {
            mpv_node_list *list = dst->u.list;
            if (list != nullptr) {
                for (int n = 0; n < list->num; n++) {
                    if (list->keys != nullptr) {
                        delete[] list->keys[n];
                    }
                    if (list->values != nullptr) {
                        free_node(&list->values[n]);
                    }
                }
                delete[] list->keys;
                delete[] list->values;
            }
            delete list;
            break;
        }
        default:;
        }
        dst->format = MPV_FORMAT_NONE;
    }
};

/**
 * RAII wrapper that calls mpv_free_node_contents() on the pointer.
 */
struct node_autofree
{
    mpv_node *ptr;
    explicit node_autofree(mpv_node *a_ptr)
        : ptr(a_ptr)
    {}
    ~node_autofree() { mpv_free_node_contents(ptr); }
};

/**
 * This is used to return error codes wrapped in QVariant for functions which
 * return QVariant.
 *
 * You can use get_error() or is_error() to extract the error status from a
 * QVariant value.
 */
struct ErrorReturn
{
    /**
     * enum mpv_error value (or a value outside of it if ABI was extended)
     */
    int error;

    ErrorReturn()
        : error(0)
    {}
    explicit ErrorReturn(int err)
        : error(err)
    {}
};

/**
 * Return the mpv error code packed into a QVariant, or 0 (success) if it's not
 * an error value.
 *
 * @return error code (<0) or success (>=0)
 */
static inline auto get_error(const QVariant &v) -> int
{
    if (!v.canConvert<ErrorReturn>()) {
        return 0;
    }
    return v.value<ErrorReturn>().error;
}

/**
 * Return whether the QVariant carries a mpv error code.
 */
static inline auto is_error(const QVariant &v) -> bool
{
    return get_error(v) < 0;
}

/**
 * Return the given property as mpv_node converted to QVariant, or QVariant()
 * on error.
 *
 * @param name the property name
 * @return the property value, or an ErrorReturn with the error code
 */
static inline auto get_property(mpv_handle *ctx, const QString &name) -> QVariant
{
    mpv_node node;
    int err = mpv_get_property(ctx, name.toUtf8().data(), MPV_FORMAT_NODE, &node);
    if (err < 0)
        return QVariant::fromValue(ErrorReturn(err));
    node_autofree f(&node);
    return node_to_variant(&node);
}

/**
 * Set the given property as mpv_node converted from the QVariant argument.
 *
 * @return mpv error code (<0 on error, >= 0 on success)
 */
static inline auto set_property(mpv_handle *ctx, const QString &name, const QVariant &v) -> int
{
    node_builder node(v);
    return mpv_set_property(ctx, name.toUtf8().data(), MPV_FORMAT_NODE, node.node());
}

static inline auto set_property_async(mpv_handle *ctx, const QString &name, const QVariant &v)
    -> int
{
    node_builder node(v);
    return mpv_set_property_async(ctx, 0, name.toUtf8().data(), MPV_FORMAT_NODE, node.node());
}

/**
 * mpv_command_node() equivalent.
 *
 * @param args command arguments, with args[0] being the command name as string
 * @return the property value, or an ErrorReturn with the error code
 */
static inline auto command(mpv_handle *ctx, const QVariant &args) -> QVariant
{
    node_builder node(args);
    mpv_node res;
    int err = mpv_command_node(ctx, node.node(), &res);
    if (err < 0)
        return QVariant::fromValue(ErrorReturn(err));
    node_autofree f(&res);
    return node_to_variant(&res);
}

static inline auto command_async(mpv_handle *ctx, const QVariant &args) -> QVariant
{
    node_builder node(args);
    int err = mpv_command_node_async(ctx, 0, node.node());
    if (err < 0)
        return QVariant::fromValue(ErrorReturn(err));
    return true;
}

static inline void command_abort_async(mpv_handle *ctx)
{
    mpv_abort_async_command(ctx, 0);
}

} // namespace qt
} // namespace mpv

Q_DECLARE_METATYPE(mpv::qt::ErrorReturn)

#endif
