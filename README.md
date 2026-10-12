# Simple STM32 HAL

The **Simple STM32 HAL** project is exactly that - a simple alternative to the HAL for simple projects.

Initially put together using any of my bare-metal code that I could find, it's currently being expanded to include as much basic functionality and support for the STM32G0xx family as I can while I work on projects and try to replace STMicroelectronics' HAL.

## Motivation

I like to understand how things work from the ground up, and rewriting the HAL using my own bare-metal implementation naturally fit that.

Further expanding on this notion, I decided to just make a whole "mini HAL" for projects where the regular HAL is just too much, a lot of unnecessary code runs most of the time to ensure maximum compatibility. When you know your hardware and your projects are simple, this isn't needed.

And thus, the **Simple STM32 HAL** was born!

## Goal

The goal isn't to replace the HAL - not by any means. The HAL has its own place and it fulfills it very well.

What this project wants to achieve is to provide a more minimal, lightweight, and easily trackable *(and thus debuggable)* driver for the supported STM32 families; that way you can easily see what's happening without jumping through tons of files and definitions.

## Disclaimer

Since this is intended for simple projects, it is not going to be a piece of code you can rely on to be bulletproof in every scenario.

If your project doesn't involve many parts, or you're just testing, this is mostly fine can you can use it without issue.

If you need something more, alternatives are the way to go!

Additionally, this library relies on CMSIS *(CMSIS 6 as of writing)* and the relevant device headers from STMicroelectronics to be present, otherwise things like register names will not be found.
