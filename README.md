ROC

Rhys's Other C

Syntax:

```
let main(): i32 {
    let x = 5i16;
    let y: u8* = &x as u8*;
    return (x + (*y as i16) * 6) as i32;
}

```
