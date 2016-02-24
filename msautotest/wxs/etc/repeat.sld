<?xml version="1.0"?>
<StyledLayerDescriptor version="1.0.0">
 <NamedLayer>
  <Name>popplace</Name>
  <UserStyle>
   <Title>xxx</Title>
   <FeatureTypeStyle>
    <Rule>
     <PointSymbolizer>
      <Geometry>
       <PropertyName>locatedAt</PropertyName>
      </Geometry>
      <Graphic>
       <Mark>
        <WellKnownName>circle</WellKnownName>
        <Fill>
         <CssParameter name="fill">#000000</CssParameter>
        </Fill>
       </Mark>
       <Size>14.0</Size>
      </Graphic>
     </PointSymbolizer>
    </Rule>
   </FeatureTypeStyle>
  </UserStyle>
 </NamedLayer>
 <NamedLayer>
  <Name>popplace</Name>
  <UserStyle>
   <Title>xxx</Title>
   <FeatureTypeStyle>
    <Rule>
     <PointSymbolizer>
      <Geometry>
       <PropertyName>locatedAt</PropertyName>
      </Geometry>
      <Graphic>
       <Mark>
        <WellKnownName>star</WellKnownName>
        <Fill>
         <CssParameter name="fill">#ff0000</CssParameter>
        </Fill>
       </Mark>
       <Size>10.0</Size>
      </Graphic>
     </PointSymbolizer>
    </Rule>
   </FeatureTypeStyle>
  </UserStyle>
 </NamedLayer>
</StyledLayerDescriptor>

