# Maintainer: taotieren <admin@taotieren.com>

pkgname=purc-git
pkgver=0.8.1.r3.gd1b31944
pkgrel=1
pkgdesc="The prime HVML interpreter for C Language."
arch=('any')
url="https://github.com/HVML/PurC"
license=('LGPL-3.0')
provides=(${pkgname})
conflicts=(${pkgname%-git})
#replaces=(${pkgname})
depends=('cmake' 'gcc' 'glibc' 'python' 'bison' 'flex')
makedepends=('git' 'cmake' 'ninja')
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
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -B build
    cmake --build build
}

package() {
    make -C "${srcdir}"/${pkgname%-git}/build install DESTDIR="${pkgdir}"
}
