Model = {
    vertices = {
        { -0.5, -0.5, -0.5 }, -- 0
        {  0.5, -0.5, -0.5 }, -- 1
        {  0.5,  0.5, -0.5 }, -- 2
        { -0.5,  0.5, -0.5 }, -- 3

        { -0.5, -0.5,  0.5 }, -- 4
        {  0.5, -0.5,  0.5 }, -- 5
        {  0.5,  0.5,  0.5 }, -- 6
        { -0.5,  0.5,  0.5 }, -- 7
    },
    colors = {
        { 1, 0, 0, 1 },
        { 1, 1, 0, 1 },
        { 0, 1, 0, 1 },
        { 0, 0, 1, 1 },
        
        { 1, 0, 1, 1 },
        { 0, 1, 1, 1 },
        { 1, 1, 1, 1 },
        { 0, 0, 0, 1 }
    },
    indices = {
        0, 1, 2, 0, 2, 3, -- front face
        4, 6, 5, 4, 7, 6, -- back face
        0, 7, 4, 0, 3, 7,
        
        3, 6, 7, 3, 2, 6,
        0, 4, 5, 0, 5, 1,
        1, 6, 2, 1, 5, 6
    }
}