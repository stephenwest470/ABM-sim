#!/usr/bin/ruby

flag = false

while line = STDIN.gets
  if (/^y:/ =~ line)
    yearary = line.strip.split(":")
    year = yearary[1]
    if (flag) # not the first time
      print "\n" + year + " "
    else
      print year + " "
      flag = true
    end
  end

  next unless (flag) 

  if ((/^area A / =~ line) or (/^area B / =~ line) or
    (/^area C / =~ line) or (/^area D / =~ line) or (/^area E / =~ line) or
    (/^area F / =~ line) or (/^area G / =~ line) or (/^area H / =~ line) or
    (/^area I / =~ line) or (/^area J / =~ line) or (/^area K / =~ line) or
    (/^area L / =~ line) or (/^area M / =~ line) or (/^area N / =~ line) or
    (/^area O / =~ line) or (/^area P / =~ line) or (/^area Q / =~ line) or
    (/^area R / =~ line) or (/^area S / =~ line) or (/^area T / =~ line) or
    (/^area U / =~ line) or (/^area V / =~ line))
    areaAry = line.strip.split(" ")
    skill = areaAry[4]
    #print area + " " + population + " " 
    print skill + " " 
  end

end
