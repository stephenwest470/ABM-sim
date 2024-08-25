#!/usr/bin/ruby

flag = false

while line = STDIN.gets
  if (/^y:/ =~ line)
    yearary = line.strip.split(":")
    year = yearary[1]
    if (flag) #not the first time
      print "\n" + year + " "
    else      
      print year + " "
      flag = true
    end
  end

  next unless (flag)

  if ((/^A,/ =~ line) or (/^B,/ =~ line) or
    (/^C,/ =~ line) or (/^D,/ =~ line) or (/^E,/ =~ line) or
    (/^F,/ =~ line) or (/^G,/ =~ line) or (/^H,/ =~ line) or
    (/^I,/ =~ line) or (/^J,/ =~ line) or (/^K,/ =~ line) or
    (/^L,/ =~ line) or (/^M,/ =~ line) or (/^N,/ =~ line) or
    (/^O,/ =~ line) or (/^P,/ =~ line) or (/^Q,/ =~ line) or
    (/^R,/ =~ line) or (/^S,/ =~ line) or (/^T,/ =~ line) or
    (/^U,/ =~ line) or (/^V,/ =~ line))
    areaAry = line.strip.split(",")
    area = areaAry[0]
    population = areaAry[257]
    #print area + " " + population + " " 
    print population + " " 
  end

end
