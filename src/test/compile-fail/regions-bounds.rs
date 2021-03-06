// Check that explicit region bounds are allowed on the various
// nominal types (but not on other types) and that they are type
// checked.

enum an_enum/& { }
iface an_iface/& { }
class a_class/& { new() { } }
resource a_rsrc/&(_x: ()) { }

fn a_fn1(e: an_enum/&a) -> an_enum/&b {
    ret e; //! ERROR mismatched types: expected `an_enum/&b` but found `an_enum/&a`
}

fn a_fn2(e: an_iface/&a) -> an_iface/&b {
    ret e; //! ERROR mismatched types: expected `an_iface/&b` but found `an_iface/&a`
}

fn a_fn3(e: a_class/&a) -> a_class/&b {
    ret e; //! ERROR mismatched types: expected `a_class/&b` but found `a_class/&a`
}

fn a_fn4(e: a_rsrc/&a) -> a_rsrc/&b {
    ret e; //! ERROR mismatched types: expected `a_rsrc/&b` but found `a_rsrc/&a`
}

fn a_fn5(e: int/&a) -> int/&b {
    //!^ ERROR Region parameters are not allowed on this type.
    //!^^ ERROR Region parameters are not allowed on this type.
    ret e;
}

fn main() { }