mod ltl;

#[cxx::bridge(namespace = "lib")]
mod ffi {
    extern "Rust" {
        fn test();
    }
}

pub fn test() {
    let f = crate::ltl::conjunction(crate::ltl::atomic(String::from("A")), crate::ltl::atomic(String::from("B")));
    let g = crate::ltl::conjunction(f, crate::ltl::atomic(String::from("C")));

    println!("{}", g);
}