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

  if (/^area_code:/ =~ line)
    areaAry = line.strip.split(",")
    gvalue = areaAry[2]
    #print area + " " + population + " " 
    print gvalue + " " 
  end

end
