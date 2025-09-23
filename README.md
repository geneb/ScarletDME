# ScarletDME

A multivalue database management system built from OpenQM 2.6. Thank you
to Martin Phillips for the original GPL release of OpenQM.

## Building from Source

Building ScarletDME requires the gcc and make.

```
git clone https://github.com/geneb/ScarletDME.git
cd ScarletDME
make
sudo make install
```

The installation will create the qmusers group and a qmsys user. 

To update a specific user to use qm, you will need to add them to the
group:

```
sudo usermod -aG qmusers username
```

You can start scarletdme by doing:

```
sudo qm -start
```

## Community

The community for multivalue is small and so here are some places specific
to ScarletDME. 

[ScarletDME Discord](https://discord.gg/H7MPapC2hK)

[ScarletDME Google Group](https://groups.google.com/g/scarletdme/)

There are also general multivalue communites online.

[Pick Google Group](https://groups.google.com/g/mvdbms)

[OpenQM Google Group](https://groups.google.com/g/openqm)

[Rocket Forums](https://community.rocketsoftware.com/forums/multivalue)
