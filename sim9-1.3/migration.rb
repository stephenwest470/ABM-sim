#!/usr/bin/ruby

flag = false
flag2 = false
allmigration = 0

while line = STDIN.gets
  if (/^y:/ =~ line)
    yearary = line.strip.split(":")
    year = yearary[1]
    flag2 = true
    if (flag) #not the first time
      print allmigration.to_s + "\n" + year + " "
    else      
      print year + " "
      flag = true
    end
  end

  next unless (flag)

  if (flag2) then 
     allmigration = 0
     flag2 = false
  end 

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
    migration1 = areaAry[263]
    if ((area == "V")) then
        migration2 = 0
    else
        migration2 = areaAry[266].to_i
    end
    migration = migration1.to_i + migration2

    print migration.to_s + " " 

    if (flag2 == false) then 
       allmigration = allmigration + migration
    end
  end

end
