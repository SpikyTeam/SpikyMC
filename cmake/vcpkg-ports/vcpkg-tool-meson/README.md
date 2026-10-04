This is a copy of the upstream vcpkg `vcpkg-tool-meson` port, bundled so we can carry patches that upstream does not have yet:

- `meson-intl.patch`
- `adjust-python-dep.patch`
- `adjust-args.patch`
- `remove-freebsd-pcfile-specialization.patch`
- `fix-libcpp-enable-assertions.patch` (upstream meson PR 14548, can be dropped once we require Meson 1.8.3)

Check whether upstream has landed any of these before touching this tree; if it has, prefer deleting the port and using the upstream one, since maintaining a vendored port means tracking every upstream change by hand.