# PurC AUR package packaging script

## PurC release version Packaging script

```bash
# The following operations are done under the Arch Linux release version.
# Other Linux distributions can be referenced.
$ cd aur/purc

# Modify the version number to the current latest release number
$ vim PKGBUILD
# eg., pkgver=0.8.1
# Save and exit

# Update the check value of the PurC release version and generate a .SRCINFO file
$ updpkgsums && makepkg --printsrcinfo > .SRCINFO

# Compile and package the PurC release version
$ makepkg -sf

# Install the PurC package
$ yay -U purc-*.tar.zst

# Compile, package and install
$ makepkg -sfi

# Online installation via AUR: [purc](https://aur.archlinux.org/packages/purc) or [Self-built software source](https://github.com/taotieren/aur-repo)
$ yay -S purc
```

## PurC development version Packaging script

```bash
# The following operations are done under the Arch Linux release version.
# Other Linux distributions can be referenced.
$ cd aur/purc-git

# The PurC development version does not need to update the check values and .SRCINFO files when compiling.
# Updates are required only when submitted to AUR.
# Update the check value of the PurC development version and generate a .SRCINFO file
$ updpkgsums && makepkg --printsrcinfo > .SRCINFO

# Compile and package the PurC development version
$ makepkg -sf

# Install the PurC package
$ yay -U purc-git-*.tar.zst

# Compile, package and install
$ makepkg -sfi

# Online installation via AUR: [purc-git](https://aur.archlinux.org/packages/purc-git) or [Self-built software source](https://github.com/taotieren/aur-repo)
$ yay -S purc-git
```
