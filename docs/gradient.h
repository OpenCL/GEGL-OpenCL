%!PS-Adobe-1.0

/paper_width  8.5 72 mul def
/paper_height 11  72 mul def

/$gradientDict 3 dict def
$gradientDict begin

% begin end t -> value
/scalar_interp {
  3 dict begin
    /t exch def
    /e exch def
    /b exch def
    t 90 mul sin dup mul e b sub mul b add
%    t e b sub mul b add
  end
} def

% t, bottom r g b, top r g b -> interp r g b
/color_interp {
  7 dict begin
    /tb exch def
    /tg exch def
    /tr exch def
    /bb exch def
    /bg exch def
    /br exch def
    /t exch def
    br tr t scalar_interp
    bg tg t scalar_interp
    bb tb t scalar_interp
  end
} def

% bottom r g b, top r g b, steps
/gradient_box {
  9 dict begin
    /steps exch def
    /tb exch def
    /tg exch def
    /tr exch def
    /bb exch def
    /bg exch def
    /br exch def
    /step 1 steps div def
    gsave
      newpath
      0 step 1 {
        /t exch def
%        t t t setrgbcolor
        t br bg bb tr tg tb color_interp setrgbcolor
        0 t moveto
        0 t step add lineto
        1 t step add lineto
        1 t lineto
        fill
      } for
    grestore
  end
} def

end

%%EndProlog
