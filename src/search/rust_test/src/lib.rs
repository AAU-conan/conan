mod ltl;

#[cxx::bridge(namespace = "lib")]
mod ffi {
    extern "Rust" {
        fn test();
    }
}

pub fn test() {
    let f = crate::ltl::conjunction(crate::ltl::atomic(8), crate::ltl::atomic(1));
    let g = crate::ltl::conjunction(f, crate::ltl::atomic(4));

    println!("{}", g);
}