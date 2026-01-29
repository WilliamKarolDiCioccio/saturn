import { useEffect, useState } from "react";
import Modal from "react-modal";

type MermaidDiagramProps = {
  name: string;
  alt?: string;
};

// Set app element for accessibility
if (typeof window !== "undefined") {
  Modal.setAppElement("body");
}

export default function MermaidDiagram({
  name,
  alt = "Mermaid Diagram",
}: MermaidDiagramProps) {
  const [theme, setTheme] = useState<"light" | "dark">("light");
  const [isOpen, setIsOpen] = useState(false);
  const [scale, setScale] = useState(1);
  const [position, setPosition] = useState({ x: 0, y: 0 });
  const [isDragging, setIsDragging] = useState(false);
  const [dragStart, setDragStart] = useState({ x: 0, y: 0 });

  useEffect(() => {
    if (typeof window === "undefined") return;

    const updateTheme = () => {
      const current = document.documentElement.getAttribute("data-theme");
      if (current === "dark" || current === "light") {
        setTheme(current);
      }
    };

    updateTheme();
    const observer = new MutationObserver(updateTheme);
    observer.observe(document.documentElement, {
      attributes: true,
      attributeFilter: ["data-theme"],
    });

    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    if (!isOpen) {
      setScale(1);
      setPosition({ x: 0, y: 0 });
    }
  }, [isOpen]);

  const handleWheel = (e: React.WheelEvent) => {
    e.preventDefault();
    const delta = e.deltaY > 0 ? 0.9 : 1.1;
    setScale((s) => Math.min(Math.max(0.5, s * delta), 5));
  };

  const handleMouseDown = (e: React.MouseEvent) => {
    setIsDragging(true);
    setDragStart({ x: e.clientX - position.x, y: e.clientY - position.y });
  };

  const handleMouseMove = (e: React.MouseEvent) => {
    if (!isDragging) return;
    setPosition({ x: e.clientX - dragStart.x, y: e.clientY - dragStart.y });
  };

  const handleMouseUp = () => setIsDragging(false);

  const src = `/src/assets/mmds/${name}_${theme}.svg`;

  return (
    <>
      <div
        className="mermaid-container"
        style={{
          display: "flex",
          justifyContent: "center",
          padding: "1rem 0",
          position: "relative",
        }}
      >
        <img
          src={src}
          alt={alt}
          style={{
            maxWidth: "100%",
            height: "auto",
            filter:
              theme === "dark"
                ? "drop-shadow(0 0 2px rgba(255,255,255,0.1))"
                : "none",
          }}
        />
        <div
          style={{
            position: "absolute",
            top: 0,
            right: 0,
            width: "100%",
            height: "100%",
            pointerEvents: "none",
          }}
          className="mermaid-hover-btn-wrapper"
        >
          <button
            onClick={() => setIsOpen(true)}
            style={{
              position: "absolute",
              bottom: "1.5rem",
              right: "1.5rem",
              background: theme === "dark" ? "#333" : "#fff",
              border: `1px solid ${theme === "dark" ? "#555" : "#ddd"}`,
              borderRadius: "4px",
              padding: "0.5rem",
              cursor: "pointer",
              fontSize: "1.2rem",
              opacity: 0.5,
              transition: "opacity 0.2s",
              pointerEvents: "auto",
            }}
            aria-label="Open fullscreen"
            className="mermaid-fullscreen-btn"
          >
            ⛶
          </button>
          <style>
            {`
              .mermaid-container:hover .mermaid-fullscreen-btn {
          opacity: 1 !important;
              }
            `}
          </style>
        </div>
      </div>

      <Modal
        isOpen={isOpen}
        onRequestClose={() => setIsOpen(false)}
        style={{
          overlay: {
            backgroundColor: "rgba(0, 0, 0, 0.7)",
            zIndex: 999999,
          },
          content: {
            background: theme === "dark" ? "#1a1a1a" : "#fff",
            border: "none",
            borderRadius: "8px",
            padding: 0,
            inset: "2rem",
            display: "flex",
            flexDirection: "column",
          },
        }}
      >
        {/* Header */}
        <div
          style={{
            padding: "1rem",
            borderBottom: `1px solid ${theme === "dark" ? "#333" : "#ddd"}`,
            display: "flex",
            justifyContent: "space-between",
            alignItems: "center",
          }}
        >
          <div style={{ display: "flex", gap: "0.5rem" }}>
            <button
              onClick={() => setScale((s) => Math.max(0.5, s * 0.8))}
              style={{
                background: theme === "dark" ? "#333" : "#f0f0f0",
                color: theme === "dark" ? "#fff" : "#000",
                border: "none",
                borderRadius: "4px",
                padding: "0.5rem 1rem",
                cursor: "pointer",
                fontSize: "1.2rem",
              }}
            >
              -
            </button>
            <button
              onClick={() => setScale((s) => Math.min(5, s * 1.2))}
              style={{
                background: theme === "dark" ? "#333" : "#f0f0f0",
                color: theme === "dark" ? "#fff" : "#000",
                border: "none",
                borderRadius: "4px",
                padding: "0.5rem 1rem",
                cursor: "pointer",
                fontSize: "1.2rem",
              }}
            >
              +
            </button>
            <button
              onClick={() => {
                setScale(1);
                setPosition({ x: 0, y: 0 });
              }}
              style={{
                background: theme === "dark" ? "#333" : "#f0f0f0",
                color: theme === "dark" ? "#fff" : "#000",
                border: "none",
                borderRadius: "4px",
                padding: "0.5rem 1rem",
                cursor: "pointer",
              }}
            >
              Reset
            </button>
          </div>
          <button
            onClick={() => setIsOpen(false)}
            style={{
              background: "transparent",
              color: theme === "dark" ? "#fff" : "#000",
              border: "none",
              cursor: "pointer",
              fontSize: "1.5rem",
            }}
          >
            ✕
          </button>
        </div>

        {/* Content */}
        <div
          onWheel={handleWheel}
          onMouseDown={handleMouseDown}
          onMouseMove={handleMouseMove}
          onMouseUp={handleMouseUp}
          onMouseLeave={handleMouseUp}
          style={{
            flex: 1,
            cursor: isDragging ? "grabbing" : "grab",
            userSelect: "none",
            display: "flex",
            alignItems: "center",
            justifyContent: "center",
            overflow: "hidden",
          }}
        >
          <img
            src={src}
            alt={alt}
            draggable={false}
            style={{
              transform: `translate(${position.x}px, ${position.y}px) scale(${scale})`,
              maxWidth: "100%",
              maxHeight: "100%",
              filter:
                theme === "dark"
                  ? "drop-shadow(0 0 2px rgba(255,255,255,0.1))"
                  : "none",
              pointerEvents: "none",
            }}
          />
        </div>
      </Modal>
    </>
  );
}
