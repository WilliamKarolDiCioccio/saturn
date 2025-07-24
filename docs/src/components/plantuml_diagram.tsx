import { useEffect, useState, useMemo } from 'react';
import plantumlEncoder from 'plantuml-encoder';

type PlantUMLDiagramProps = {
  code: string;
  alt?: string;
};

// Cache outside component to persist across re-renders
const diagramCache = new Map<string, string>();

export default function PlantUMLDiagram({ code, alt = 'PlantUML Diagram' }: PlantUMLDiagramProps) {
  const [theme, setTheme] = useState<'light' | 'dark'>('light');

  useEffect(() => {
    if (typeof window === 'undefined') return;
    
    const updateTheme = () => {
      const current = document.documentElement.getAttribute('data-theme');
      if (current === 'dark' || current === 'light') {
        setTheme(current);
      }
    };

    updateTheme();
    const observer = new MutationObserver(updateTheme);
    observer.observe(document.documentElement, {
      attributes: true,
      attributeFilter: ['data-theme'],
    });

    return () => observer.disconnect();
  }, []);

  const src = useMemo(() => {
    const darkSkin = `
skinparam backgroundColor #f5f5f5
skinparam monochrome reverse
`.trim();

    const fullSource = theme === 'dark' 
      ? `@startuml\n${darkSkin}\n${code.replace('@startuml', '').replace('@enduml', '')}\n@enduml`
      : code;

    // Create cache key from source
    const cacheKey = `${theme}-${fullSource}`;
    
    if (diagramCache.has(cacheKey)) {
      return diagramCache.get(cacheKey)!;
    }

    const encoded = plantumlEncoder.encode(fullSource);
    // Changed from /png/ to /svg/ for SVG output
    const url = `https://www.plantuml.com/plantuml/svg/${encoded}`;
    
    diagramCache.set(cacheKey, url);
    return url;
  }, [code, theme]);

  return (
    <img 
      src={src} 
      alt={alt} 
      style={{ 
        maxWidth: '100%', 
        borderRadius: '0.5rem',
        // SVGs scale nicely, but this ensures crisp rendering
        imageRendering: 'auto'
      }} 
    />
  );
}