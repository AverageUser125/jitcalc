# JITcalc
 
SHOULD convert an equation like:
```
2x + 5
```
into a callable function like:
func(5) - > 12


Additionally will graph the equation you put in and will let you zoom out and move around using the mouse

# Techincal detailes
- To convert the equation string into a runnable function I use LLVM, orcjit
- OpenGL is used to render the graphs and everything
- ImGUI is used to display the equation widget

# Disclamer
This may, or will, absolutely fail horribly and there is zero
guarantee it will compile nor run.

# Credits
- https://github.com/PixelRifts/math-expr-evaluator
- https://github.com/tsoding/arena/
- https://github.com/meemknight/cmakeSetup
- https://github.com/meemknight/gl2d
- Roboto Mono font