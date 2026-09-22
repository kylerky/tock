fn main() {
    println!("cargo::rerun-if-changed=runtime.c");
    println!("cargo::rerun-if-changed=build.rs");

    cc::Build::new()
        .file("runtime.c")
        .compile("runtime");

    tock_build_scripts::default_linker_script();
}
