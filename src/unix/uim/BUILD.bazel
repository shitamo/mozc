load(
    "//:build_defs.bzl",
    "mozc_cc_binary",
    "mozc_cc_library",
)

package(default_visibility = ["//:__subpackages__"])

mozc_cc_library(
    name = "uim_key_translator",
    srcs = [
        "key_translator.cc",
    ],
    hdrs = [
        "key_translator.h",
    ],
    deps = [
        "//base:port",
        "//base:vlog",
        "//client",
        "//protocol:commands_cc_proto",
        "//protocol:config_cc_proto",
        "@com_google_absl//absl/log:check",
        "@uim",
    ],
)

mozc_cc_binary(
    name = "libuim-mozc.so",
    srcs = ["mozc.cc"],
    linkshared = 1,
    linkstatic = 1,
    deps = [
        ":uim_key_translator",
        "//base:init_mozc",
        "//client",
        "//protocol:commands_cc_proto",
        "//protocol:config_cc_proto",
        "@uim",
    ],
)
