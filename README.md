# [Umibōzu](https://en.wikipedia.org/wiki/Umib%C5%8Dzu) (海坊主)

Gameboy Color emulator written in C++20.
Supports both Game Boy and Game Boy Color games!

(audio soon™....)

## Features

Umibozu supports features such as, but not limited to:

- [ ] 5 save states
- [ ] Full RTC support
- [ ] GBC-support


## NOTE

This emulator is far from cycle-accurate. It was not developed with accuracy in mind, nevertheless, most games are playable!

## Mappers

Support all mainline mappers:

[x] MBC1
[x] MBC3 (incl. MBC30 up to 512 banks!)
[x] MBC5

## why?

This is my first project I've ever written in C++.

you might be asking: "there are so many gameboy emulators out there! why make *another one*?"

the reason i'm making this is to **learn!**

i've always been intrigued by how emulators work, this would be an awesome learning experience. at the same time, this project will also be used to learn C++.

## Resources


I used the following resources to create this emulator:

### [Pan Docs](https://gbdev.io/pandocs/)
### [E. Haskins (DAA instruction)](https://ehaskins.com/2018-01-30%20Z80%20DAA/)
### [sm83-test-data](https://github.com/adtennant/sm83-test-data)


# Controls
| Original | Keyboard |
|----------|---------------------|
| Down     |  Down Key           |
| Up       | Up Key              |
| Left     | Left Key            |
| Right    | Right Key           |
| Start    | Enter               |
| Select   | BACKSPACE           |
| B        | A                   |
| A        | Z                   |


Don't like the controls? You can rebind them in the settings.

## Binaries

Binaries are available in the Releases tab.

Want to build from source? Check out the build section.

## Known Issues

- [ ] Pokemon Crystal seems to have graphical artifacts when interacting with NPCs -- not sure why (looking into it)

## Issues
Did you run into a crash/bug? Feel free to open a ticket, and I'll have a look.

## Screenshots
![](assets/Screenshot%20from%202023-12-31%2002-55-40.png)
![](assets/Screenshot%20from%202023-12-31%2003-29-17.png)
![](assets/Screenshot%20from%202023-12-31%2003-27-07.png)