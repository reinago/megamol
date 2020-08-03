
    vec4 inPos = vec4(applyInstancing(inPosition.xyz), 1.0);
    //inPos.w = 1.0;

#ifdef WITH_SCALING
    rad *= scaling;
#endif // WITH_SCALING
    
    squarRad = rad * rad;

 #ifdef HALO
    squarRad = (rad + HALO_RAD) * (rad + HALO_RAD);
#endif // HALO
