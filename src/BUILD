
cc_library(
	name = "build-all-the-files",
	srcs = glob(["*.cc"]),
	hdrs = glob(["*.h"]),
	copts = ["-w -g"],
)

cc_binary(
	name = "maui",
	srcs = ["maui.cc"],
	deps = [
				":build-all-the-files",
			],
	copts = ["-w -g"],
)
