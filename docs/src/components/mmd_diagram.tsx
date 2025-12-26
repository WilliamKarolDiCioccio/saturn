import { useEffect, useState } from 'react';

type MermaidDiagramProps = {
  name: string;
  alt?: string;
};

export default function MermaidDiagram({ name, alt = 'Mermaid Diagram' }: MermaidDiagramProps) {
  const [theme, setTheme] = useState<'light' | 'dark'>('light');

  useEffect(() => {
    if (typeof window === 'undefined') return;

    // Check data-theme on <html> (specific to Starlight theme switcher)
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

  // Construct path based on the generated files in src/assets/mmds
  const src = `/src/assets/mmds/${name}_${theme}.svg`;

  return (
    <div className="mermaid-container" style={{ display: 'flex', justifyContent: 'center', padding: '1rem 0' }}>
      <img
        src={src}
        alt={alt}
        style={{
          maxWidth: '100%',
          height: 'auto',
          filter: theme === 'dark' ? 'drop-shadow(0 0 2px rgba(255,255,255,0.1))' : 'none'
        }}
      />
    </div>
  );
}
