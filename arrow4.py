import numpy as np


vertices = [
    (0.05, 0.05, 0.0),    
    (0.05, -0.05, 0.0),   
    (-0.05, -0.05, 0.0),  
    (-0.05, 0.05, 0.0),   
    (0.05, 0.05, 0.8),    
    (0.05, -0.05, 0.8),   
    (-0.05, -0.05, 0.8),  
    (-0.05, 0.05, 0.8),   
    (0.0, 0.0, 1.0)       
]


faces = [
    [1, 2, 6], [6, 5, 1],  
    [2, 3, 7], [7, 6, 2],  
    [3, 4, 8], [8, 7, 3],  
    [4, 1, 5], [5, 8, 4],  
    
    [1, 4, 3], [3, 2, 1],  

    [5, 6, 9],     
    [6, 7, 9],     
    [7, 8, 9],     
    [8, 5, 9]      
]


vertex_normals = [(0.0, 0.0, 0.0)] * len(vertices)

for face in faces:
    v1_idx, v2_idx, v3_idx = face
    v1 = np.array(vertices[v1_idx-1])
    v2 = np.array(vertices[v2_idx-1])
    v3 = np.array(vertices[v3_idx-1])

    
    normal = np.cross(v2 - v1, v3 - v1)
    normal = normal / np.linalg.norm(normal) 

    
    for v_idx in face:
        vertex_normals[v_idx-1] = tuple(np.array(vertex_normals[v_idx-1]) + normal)


vertex_normals = [tuple(np.array(vn) / np.linalg.norm(vn)) for vn in vertex_normals]



texture_coords = []
for v in vertices:
    u = (v[0] + 0.05) / 0.1  
    v_coord = (v[1] + 0.05) / 0.1 
    texture_coords.append((u, v_coord))



with open('axis_arrow.obj', 'w') as f:
    f.write('o axis_arrow\n')

    for v in vertices:
        f.write(f'v {v[0]} {v[1]} {v[2]}\n')

    for vn in vertex_normals:
        f.write(f'vn {vn[0]} {vn[1]} {vn[2]}\n')

    for vt in texture_coords:
        f.write(f'vt {vt[0]} {vt[1]}\n')

    for face in faces:
        face_str_list = []
        for v_index in face:
            face_str_list.append(f'{v_index}/{v_index}/{v_index}') 
        f.write('f ' + ' '.join(face_str_list) + '\n')

print("axis_arrow.obj file generated.")
