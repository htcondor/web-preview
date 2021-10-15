// This page hosts the code that is used on every single page.

// If your js function does not need to be on every page don't put it here!

window.onload = () => {
    const MainSearchBar = new SearchBar("main-search-bar",
        "{{ 'assets/search/main_index.json' | relative_url }}",
        "{{ 'assets/search/main_metadata.json' | relative_url }}")
}