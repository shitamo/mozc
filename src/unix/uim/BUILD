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
        "//protocol:config_cc_proto",
        "//protocol:commands_cc_proto",
        "@com_google_absl//absl/log:check",
        "//client:client",
        "@uim//:uim",
    ],
)

mozc_cc_binary(
    name = "libuim-mozc.so",
    srcs = ["mozc.cc"],
    deps = [
        "//base:init_mozc",
        "//protocol:config_cc_proto",
        "//protocol:commands_cc_proto",
        "//client:client",
        ":uim_key_translator",
        "@uim//:uim",
    ],
    linkstatic = 1,
    linkshared = 1,
)

