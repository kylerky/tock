fn main() {
    println!("cargo::rerun-if-changed=runtime.c");

    cc::Build::new()
        .file("runtime.c")
        .compiler("clang")
        .compile("runtime");

    tock_build_scripts::default_linker_script();
}
