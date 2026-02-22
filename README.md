# MDP_CA1 - Michal Becmer/D00256088/GD4A
### Disclaimer!!!
This project was done solo, therefore all additional code is written by me and with occasional help from github's copilot for students (the agent used was Claude Sonnet 4.5).
### Game Description
A 2D physics-based shooter game inspired mainly by Rounds. The core gameplay loop is designed around short competitive rounds between two players. The game focuses on fast-paced rounds, responsive controls, and physics-based movement.

## Current Issue/Bugs
- Dust particles randomly "connect" between players
- Collisions sometimes fail whenever game is loaded, they work when game is relaunched (this happens rarely)
- Re-mapping controls works only for keyboard (no compatability with mouse or gamepad)

## AI Usage
- I used Github Copilot with the integrated Claude Sonnet 4.5 Model
- #### THE AI WAS NEVER USED TO WRITE ENTIRE FILES OR FEATURES IT WAS USED AS A TOOL!!!
- The Agent was used to:
  - Assist with bug fixes (exp. Gun rotation formula was not correctly written and didn't allow for the gun to be rotate both ways)
  - Explain code that I didn't understand (From the original repository, this helped me to expand on the code from class then myself.)
  - Assist with writing more complex code (exp. Player input binding, I couldn't find any proper resources for more complex features like this that were not outdated)

## References
- ### Inspiration:
  - https://store.steampowered.com/app/1557740/ROUNDS/
- ### Github Copilot (Student):
  - https://github.com/education/students
- ### Network Discussion:
  - https://studentdkit-my.sharepoint.com/:w:/g/personal/d00256088_student_dkit_ie/IQBgn6McPDNJSrFpiSBBLzAdAXBIbShsGV7xgL1y0h50X8Q?e=AUL8cQ
- ### Coding resources:
  - Physics:
    - Semi-Implicit Euler Integration: https://gafferongames.com/post/integration_basics/
  - Controller/Inputs:
    - SFML Joystick Example (github): https://github.com/SFML/SFML/blob/3.0.0/examples/joystick/Joystick.cpp
  - Collisions:
    - Platform Collisions: https://code.tutsplus.com/collision-detection-using-the-separating-axis-theorem--gamedev-169t  
- ### Graphics:
  - Spritesheet - https://kenney.nl/assets/scribble-platformer
  - Chromatic Abberation Shader: https://www.shadertoy.com/view/Mds3zn
  - Screen Shake Shader: https://www.shadertoy.com/view/tdSyWz
  - Shader Website: https://www.shadertoy.com/
- ### Audio:
  - StartGame - https://freesound.org/people/Breviceps/sounds/452998/
  - Error - https://freesound.org/people/Sadiquecat/sounds/794316/
  - PairedPlayer - https://freesound.org/people/shaman32/sounds/840274/
  - ButtonClick - https://freesound.org/people/Funky_Audio/sounds/703000/
  - Jump SFX:
    - Jump_C_01 by cabled_mess -- https://freesound.org/s/350902/ -- License: Creative Commons 0
    - Jump_C_05 by cabled_mess -- https://freesound.org/s/350905/ -- License: Creative Commons 0
    - Jump_C_04 by cabled_mess -- https://freesound.org/s/350906/ -- License: Creative Commons 0
    - Jump_C_03 by cabled_mess -- https://freesound.org/s/350903/ -- License: Creative Commons 0
    - Jump_C_08 by cabled_mess -- https://freesound.org/s/350898/ -- License: Creative Commons 0
    - Jump_Landing_Wood_01.wav by IPaddeh -- https://freesound.org/s/422857/ -- License: Creative Commons 0
    - Jump_Landing_Wood_02.wav by IPaddeh -- https://freesound.org/s/422869/ -- License: Creative Commons 0
  - GunShot - Damage sound effect by Raclure -- https://freesound.org/s/458867/ -- License: Creative Commons 0
  - Player Hit:
    - Retro video game sfx - Punch by OwlStorm -- https://freesound.org/s/404723/ -- License: Creative Commons 0
    - Retro video game sfx - Punch 2 by OwlStorm -- https://freesound.org/s/404766/ -- License: Creative Commons 0
  - Death: man_hurt_2.wav by cdakak -- https://freesound.org/s/641689/ -- License: Creative Commons 0
