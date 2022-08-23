# Maintainer: taotieren <admin@taotieren.com>

pkgname=purc
pkgver=0.8.1
pkgrel=1
pkgdesc="The prime HVML interpreter for C Language."
arch=('any')
url="https://github.com/HVML/PurC"
license=('LGPL-3.0')
provides=(${pkgname} 'PurC')
conflicts=(${pkgname}-git)
#replaces=(${pkgname})
depends=('cmake' 'gcc' 'glibc' 'python' 'bison' 'flex')
makedepends=('cmake' 'ninja')
backup=()
options=('!strip')
#install=${pkgname}.install
source=("${pkgname}-${pkgver}.tar.gz::${url}/archive/refs/tags/ver-${pkgver}.tar.gz")
sha256sums=('387fc10560eed813cd92b36a224f17a7fa94c2aa20264f7e75c9c471505071d5')

build() {
    cd "${srcdir}/PurC-ver-${pkgver}/"
# CMake build
#     cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -B build
#     cmake --build build

# Ninja build
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT=Linux -B build -G Ninja
    ninja -C build
}

package() {
# make install
#     make -C "${srcdir}"/PurC-ver-${pkgver}/build install DESTDIR="${pkgdir}"

# ninja install
    DESTDIR="${pkgdir}" ninja -C "${srcdir}"/PurC-ver-${pkgver}/build install
}
