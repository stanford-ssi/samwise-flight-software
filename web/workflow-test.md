# random-space-art.py
#!/usr/bin/env python3
import random

ARTS = [
r"""
    /\ 
     /  \   _  
    /----\ / \ 
   /      \   \
  /        \___\
  |  ROCKET   |
  '-----------'
""",
r"""
    .-.
   .-(   )-.
  /  (   )  \
  \   '-'   /
   '-.___.-'
    /  |  \
   '   |   '
     '
   Planet X
""",
r"""
    .--.
     / _.-'
    / /   
   / /    
  /_/     
 ( )  .-.
  Y  (o o)
    |=|
     __|__
    /  |  \
   '   |   '
 Astronaut
""",
r"""
    .-.
     (   ).
    (___(__)
     ' ' ' '
   ~ UFO ~
""",
]

def main():
    print(random.choice(ARTS))

if __name__ == "__main__":
    main()