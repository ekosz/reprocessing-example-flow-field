open Reprocessing;

/** A small 2D Vector library. Code was mostly taken from the P5.js Vector code */
module Vector = {
  type t = {
    x: float,
    y: float
  };
  let create = (~x=0., ~y=0., ()) : t => {x, y};
  let add = (a: t, b: t) : t => {x: a.x +. b.x, y: a.y +. b.y};
  let mult = (n: float, vec: t) : t => {x: vec.x *. n, y: vec.y *. n};
  let div = (n: float, vec: t) : t => {x: vec.x /. n, y: vec.y /. n};
  let magSq = (vec: t) => {
    let {x, y} = vec;
    x *. x +. y *. y;
  };
  let mag = (vec: t) => sqrt(magSq(vec));
  let from_angle = (angle, ~length=1., ()) : t => {
    x: length *. cos(angle),
    y: length *. sin(angle)
  };
  let heading = (vec: t) => atan2(vec.x, vec.y);
  let limit = (max: float, vec: t) : t => {
    let mSq = magSq(vec);
    if (mSq > max *. max) {
      vec |> div(sqrt(mSq)) |> mult(max);
    } else {
      vec;
    };
  };
  let normolize = (vec: t) : t => {
    let mag = mag(vec);
    mag == 0. ? vec : div(mag, vec);
  };
  let setMag = (n: float, vec: t) : t => vec |> normolize |> mult(n);
};

/** How big are the force squares */
let scl = 10;

/** How does one translate pos to noise offset */
let offScl = 0.025;

/** How much to change per tick */
let inc = 0.01;

/** Max particle speed */
let maxSpeed = 4.0;

/** How many particles */
let particleCount = 10000;

/** How strong is the force */
let magnitude = 0.3;

/** Width of the screen */
let width = 1440;

/** Height of the screen */
let height = 900;

type particleT = {
  lastPos: Vector.t,
  pos: Vector.t,
  vel: Vector.t,
  acc: Vector.t
};

type stateT = {
  zOff: float,
  showGrid: bool,
  showColors: bool,
  particles: array(particleT)
};

let createParticles = env =>
  Array.init(
    particleCount,
    (_) => {
      let x = float_of_int(Utils.random(~min=0, ~max=Env.width(env)));
      let y = float_of_int(Utils.random(~min=0, ~max=Env.height(env)));
      {
        lastPos: Vector.create(~x, ~y, ()),
        pos: Vector.create(~x, ~y, ()),
        vel: Vector.create(),
        acc: Vector.create()
      };
    }
  );

let setup = env => {
  Env.size(~width, ~height, env);
  let particles = createParticles(env);
  Draw.background(Constants.white, env);
  {zOff: 0., particles, showGrid: false, showColors: false};
};

let vectorFromPoint = ((x: float, y: float), zOff: float) : Vector.t => {
  let xOff = floor(x /. float_of_int(scl)) *. offScl;
  let yOff = floor(y /. float_of_int(scl)) *. offScl;
  let angle = Utils.noise(xOff, yOff, zOff) *. Constants.two_pi *. 4.;
  Vector.from_angle(angle, ()) |> Vector.setMag(magnitude);
};

let applyForce = (zOff, par) : particleT => {
  let force = vectorFromPoint((par.pos.x, par.pos.y), zOff);
  {...par, acc: Vector.add(force, par.acc)};
};

let addVel = par : particleT => {
  ...par,
  vel: Vector.add(par.vel, par.acc) |> Vector.limit(maxSpeed)
};

let constrain = (value: float, limit: float) : float =>
  if (value > limit) {
    value -. limit;
  } else if (value < 0.) {
    limit +. value;
  } else {
    value;
  };

let updatePos = (width, height, par) : particleT => {
  let newPos = Vector.add(par.pos, par.vel);
  let contrainedPos =
    Vector.create(
      ~x=constrain(newPos.x, float_of_int(width)),
      ~y=constrain(newPos.y, float_of_int(height)),
      ()
    );
  {
    ...par,
    lastPos:
      newPos.x != contrainedPos.x || newPos.y != contrainedPos.y ?
        contrainedPos : par.pos,
    pos: contrainedPos
  };
};

let killAcc = (par: particleT) : particleT => {...par, acc: Vector.create()};

let genColorNum = (offset: float) =>
  Utils.remapf(
    ~value=Utils.noise(0., 0., offset),
    ~low1=0.,
    ~low2=0.,
    ~high1=1.,
    ~high2=255.
  )
  |> floor
  |> int_of_float;

let genColor = state =>
  switch (state.showColors, state.zOff) {
  | (false, _) => Utils.color(~r=0, ~g=0, ~b=0, ~a=5)
  | (true, offset) =>
    let colorOffset = offset /. 20.;
    Utils.color(
      ~r=genColorNum(colorOffset),
      ~g=genColorNum(colorOffset +. 100.),
      ~b=genColorNum(colorOffset +. 200.),
      ~a=5
    );
  };

let drawForceAt = (~x, ~y, ~zOff, env) => {
  let v = vectorFromPoint((x, y), zOff);
  Draw.pushMatrix(env);
  Draw.translate(~x, ~y, env);
  Draw.rotate(Vector.heading(v), env);
  Draw.line(~p1=(0, 0), ~p2=(scl, 0), env);
  Draw.popMatrix(env);
  ();
};

let drawParticle = (env, par) => {
  Draw.linef(
    ~p1=(par.pos.x, par.pos.y),
    ~p2=(par.lastPos.x, par.lastPos.y),
    env
  );
  ();
};

let draw = (state, env) => {
  if (state.showGrid == true) {
    /* Draw the grid of forces */
    Draw.background(Constants.white, env);
    Draw.strokeWeight(1, env);
    Draw.stroke(Utils.color(~r=0, ~g=0, ~b=0, ~a=50), env);
    for (row in 0 to Env.height(env) / scl) {
      for (col in 0 to Env.width(env) / scl) {
        drawForceAt(
          ~x=float_of_int(row * scl),
          ~y=float_of_int(col * scl),
          ~zOff=state.zOff,
          env
        );
      };
    };
  } else {
    /* Draw each particle in the field */
    Draw.strokeCap(Square, env);
    Draw.stroke(genColor(state), env);
    Array.iter(drawParticle(env), state.particles);
  };
  /* Update all of the particles */
  let particles =
    Array.map(
      par =>
        par
        |> applyForce(state.zOff)
        |> addVel
        |> updatePos(Env.width(env), Env.height(env))
        |> killAcc,
      state.particles
    );
  {...state, particles, zOff: state.zOff +. inc};
};

let keyPressed = (state, env) =>
  Events.(
    switch (Env.keyCode(env)) {
    | R =>
      Draw.background(Constants.white, env);
      {...state, particles: createParticles(env), zOff: state.zOff +. 100.};
    | G =>
      Draw.background(Constants.white, env);
      {...state, showGrid: ! state.showGrid};
    | C => {...state, showColors: ! state.showColors}
    | _ => state
    }
  );

run(~setup, ~draw, ~keyPressed, ());