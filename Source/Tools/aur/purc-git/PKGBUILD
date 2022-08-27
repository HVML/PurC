# Maintainer: taotieren <admin@taotieren.com>

pkgname=purc-git
pkgver=0.8.1.r53.g0ff03e64
pkgrel=1
pkgdesc="The prime HVML interpreter for C Language."
arch=('any')
url="https://github.com/HVML/PurC"
license=('LGPL-3.0')
groups=('hvml-git')
provides=(${pkgname%-git} 'PurC')
conflicts=(${pkgname%-git})
#replaces=(${pkgname})
depends=('glib2' 'bison' 'flex')
makedepends=('git' 'cmake' 'ninja' 'ccache' 'gcc' 'python' 'libxml2' 'ruby' 'curl' 'openssl' 'sqlite' 'pkgconf' 'zlib' 'icu')
optdepends=('domruler: DOM Ruler is a library to maintain a DOM tree, lay out and stylize the DOM elements by using CSS.'
            'purc-fetcher: The remote data fetcher for PurC.'
            'purc-midnight-commander: A generic HVML renderer in text mode for development and debugging.'
            'webkit2gtk-hvml: Web content engine for GTK (HVML)'
            'xguipro: xGUI (the X Graphics User Interface) Pro is a modern, cross-platform, and advanced HVML renderer which is based on tailored WebKit.')
backup=()
options=('!strip')
#install=${pkgname}.install
source=("${pkgname%-git}::git+${url}.git")
sha256sums=('SKIP')

pkgver() {
    cd "${srcdir}/${pkgname%-git}/"
    git describe --long --tags | sed 's/ver.//g;s/\([^-]*-g\)/r\1/;s/-/./g'
}

build() {
    cd "${srcdir}/${pkgname%-git}"

# CMake build
#     cmake -DCMAKE_BUILD_TYPE=Debug \
#         -DPORT=Linux \
#         -DCMAKE_INSTALL_PREFIX=/usr \
#         -DCMAKE_INSTALL_LIBDIR=lib \
#         -DCMAKE_INSTALL_LIBEXECDIR=lib \
#         -B build

#     cmake --build build

# Ninja build
    cmake -DCMAKE_BUILD_TYPE=Debug \
        -DPORT=Linux \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_INSTALL_LIBDIR=lib \
        -DCMAKE_INSTALL_LIBEXECDIR=lib \
        -B build \
        -G Ninja

    ninja -C build
}

package() {
# make install
#     make -C "${srcdir}"/${pkgname%-git}/build install DESTDIR="${pkgdir}"

# ninja install
    DESTDIR="${pkgdir}" ninja -C "${srcdir}"/${pkgname%-git}/build install
}
