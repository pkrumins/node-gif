{'targets': [
    {
        'target_name': 'giflib',
        'type': 'static_library',
        'standalone_static_library': 1,
        'sources': [
            'dgif_lib.c',
            'egif_lib.c',
            'gif_err.c',
            'gif_font.c',
            'gif_hash.c',
            'gifalloc.c',
            'quantize.c'
        ],
        'include_dirs': [
            '.',
        ],
        'direct_dependent_settings': {
            'include_dirs': [
                '.',
            ],
        },
    }
]}
