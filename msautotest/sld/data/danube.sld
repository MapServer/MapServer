<StyledLayerDescriptor version="1.1.0">
  <NamedLayer>
    <Name>danube</Name>
    <UserStyle>
      <FeatureTypeStyle>
        <Rule>
          <LineSymbolizer>
            <Stroke>
              <SvgParameter name="stroke-width">5</SvgParameter>
              <SvgParameter name="stroke">#0000FF</SvgParameter>
            </Stroke>
          </LineSymbolizer>
          <LineSymbolizer>
            <Stroke>
              <GraphicStroke>
                <Graphic>
                  <Size>10</Size>
                  <Mark>
                    <WellKnownName>circle</WellKnownName>
                    <Stroke>
                      <SvgParameter name="stroke">#FFFF00</SvgParameter>
                      <SvgParameter name="stroke-width">2</SvgParameter>
                    </Stroke>
                  </Mark>
                </Graphic>
                <Gap>80</Gap>
                <InitialGap>40</InitialGap>
              </GraphicStroke>
            </Stroke>
          </LineSymbolizer>
          <LineSymbolizer>
            <Stroke>
              <GraphicStroke>
                <Graphic>
                  <Size>30</Size>
                  <ExternalGraphic>
                    <OnlineResource xlink:type="simple" xlink:href="/sld/data/ship.svg" />
                    <!--
                    <OnlineResource xlink:type="simple" xlink:href="ship.svg" />
                    <OnlineResource xlink:type="simple" xlink:href="http://localhost:8000/sld/data/ship.svg" />
                    <OnlineResource xlink:type="simple" xlink:href="/sld/data/ship.svg" />
                    -->
                    <Format>image/svg+xml</Format>
                  </ExternalGraphic>
                </Graphic>
                <Gap>80</Gap>
                <InitialGap>0</InitialGap>
              </GraphicStroke>
            </Stroke>
          </LineSymbolizer>
        </Rule>
      </FeatureTypeStyle>
    </UserStyle>
  </NamedLayer>
</StyledLayerDescriptor>
