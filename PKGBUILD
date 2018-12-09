# Maintainer: alcubierre-drive <https://github.com/alcubierre-drive>
pkgname=BacklightTooler
_gitname=backlight-tooler
pkgver=r5.826f027
pkgrel=1
pkgdesc="A collection of tools to control backlight via webcam."
arch=('any')
url="https://github.com/alcubierre-drive/backlight-tooler"
license=('GPL')
depends=('systemd' 'glibc')
makedepends=('git' 'gcc' 'make')
source=("git://github.com/alcubierre-drive/${_gitname}.git")
md5sums=('SKIP')

pkgver() {
    cd "$_gitname"
    printf "r%s.%s" "$(git rev-list --count HEAD)" "$(git rev-parse --short HEAD)"
}

prepare() {
    : Nothing
}

build() {
    cd "${srcdir}/${_gitname}"
    make
}

check() {
    : Nothing
}

package() {
    cd "${srcdir}/${_gitname}"
    PREFIX="$pkgdir/" make install
}
