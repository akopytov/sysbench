for i=1, 100 do
    mytable = {1,2,3}
end

function tableinitsuccess(tab)
    return ((tab[1] == 1 and tab[2] == 2 and tab[3] == 3) or false)
end

assert(tableinitsuccess(mytable), "fail to duplicate table")
